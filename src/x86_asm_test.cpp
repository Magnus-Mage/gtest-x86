/**
 * @file x86_asm_test.cpp
 * @brief Implementation of the x86 assembly testing framework
 */

#include "x86_asm_test.h"
#include <stdexcept>
#include <sstream>
#include <algorithm>
#include <ranges>
#include <cstdlib>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <format>

namespace x86_asm_test {

bool ExpectedOutput::matches(const ExecutionResult& result) const noexcept {
    // Check exit code if specified
    if (expected_exit_code_.has_value() && result.exit_code != expected_exit_code_.value()) {
        return false;
    }
    
    // Check exact stdout match
    if (exact_stdout_.has_value() && result.stdout_output != exact_stdout_.value()) {
        return false;
    }
    
    // Check exact stderr match
    if (exact_stderr_.has_value() && result.stderr_output != exact_stderr_.value()) {
        return false;
    }
    
    // Check stdout contains patterns
    for (const auto& pattern : stdout_contains_) {
        if (result.stdout_output.find(pattern) == std::string::npos) {
            return false;
        }
    }
    
    // Check stderr contains patterns
    for (const auto& pattern : stderr_contains_) {
        if (result.stderr_output.find(pattern) == std::string::npos) {
            return false;
        }
    }
    
    return true;
}

std::string ExpectedOutput::get_mismatch_description(const ExecutionResult& result) const {
    std::ostringstream oss;
    
    if (expected_exit_code_.has_value() && result.exit_code != expected_exit_code_.value()) {
        oss << std::format("Exit code mismatch: expected {}, got {}\n", 
                          expected_exit_code_.value(), result.exit_code);
    }
    
    if (exact_stdout_.has_value() && result.stdout_output != exact_stdout_.value()) {
        oss << std::format("Stdout mismatch:\nExpected: '{}'\nActual: '{}'\n",
                          exact_stdout_.value(), result.stdout_output);
    }
    
    if (exact_stderr_.has_value() && result.stderr_output != exact_stderr_.value()) {
        oss << std::format("Stderr mismatch:\nExpected: '{}'\nActual: '{}'\n",
                          exact_stderr_.value(), result.stderr_output);
    }
    
    // Check for missing patterns in stdout
    for (const auto& pattern : stdout_contains_) {
        if (result.stdout_output.find(pattern) == std::string::npos) {
            oss << std::format("Stdout missing pattern: '{}'\nActual stdout: '{}'\n",
                              pattern, result.stdout_output);
        }
    }
    
    // Check for missing patterns in stderr
    for (const auto& pattern : stderr_contains_) {
        if (result.stderr_output.find(pattern) == std::string::npos) {
            oss << std::format("Stderr missing pattern: '{}'\nActual stderr: '{}'\n",
                              pattern, result.stderr_output);
        }
    }
    
    return oss.str();
}

AsmTestRunner::AsmTestRunner(
    std::filesystem::path executable_path,
    AsmSyntax syntax,
    TestConfig config
) : executable_path_{std::move(executable_path)}, 
    syntax_{syntax}, 
    config_{std::move(config)} {
    
    // Validate executable exists and is executable
    if (!std::filesystem::exists(executable_path_)) {
        throw std::runtime_error(
            std::format("Executable not found: {}", executable_path_.string())
        );
    }
    
    if (!std::filesystem::is_regular_file(executable_path_)) {
        throw std::runtime_error(
            std::format("Path is not a regular file: {}", executable_path_.string())
        );
    }
    
    // Check if file is executable (Unix-specific)
    if (access(executable_path_.c_str(), X_OK) != 0) {
        throw std::runtime_error(
            std::format("File is not executable: {}", executable_path_.string())
        );
    }
}

ExecutionResult AsmTestRunner::execute_process(
    std::span<const std::string> args,
    const std::optional<std::string>& stdin_data
) const {
    
    if (config_.use_strace) {
        return execute_with_strace(args, stdin_data);
    }
    
    ExecutionResult result;
    auto start_time = std::chrono::steady_clock::now();
    
    // Create pipes for stdout, stderr, and stdin
    int stdout_pipe[2], stderr_pipe[2], stdin_pipe[2];
    
    if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1 || pipe(stdin_pipe) == -1) {
        throw std::runtime_error("Failed to create pipes");
    }
    
    // Prepare arguments for execv
    std::vector<char*> exec_args;
    exec_args.reserve(args.size() + 2);
    exec_args.push_back(const_cast<char*>(executable_path_.c_str()));
    
    for (const auto& arg : args) {
        exec_args.push_back(const_cast<char*>(arg.c_str()));
    }
    exec_args.push_back(nullptr);
    
    pid_t pid = fork();
    
    if (pid == -1) {
        // Fork failed - cleanup pipes
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        throw std::runtime_error("Fork failed");
    }
    
    if (pid == 0) {
        // Child process
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        dup2(stdin_pipe[0], STDIN_FILENO);
        
        // Close all pipe file descriptors in child
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        
        // Change working directory if specified
        if (config_.working_directory != std::filesystem::current_path()) {
            if (chdir(config_.working_directory.c_str()) != 0) {
                perror("chdir");
                _exit(127);
            }
        }
        
        // Execute the program
        execv(executable_path_.c_str(), exec_args.data());
        perror("execv");
        _exit(127);
    } else {
        // Parent process
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);
        close(stdin_pipe[0]);
        
        // Write stdin data if provided
        if (stdin_data.has_value() && !stdin_data->empty()) {
            ssize_t written = write(stdin_pipe[1], stdin_data->c_str(), stdin_data->size());
            (void)written; // Suppress unused variable warning
        }
        close(stdin_pipe[1]);
        
        // Set up for non-blocking reads with timeout
        fd_set read_fds;
        struct timeval timeout_val;
        
        // Read stdout and stderr
        char buffer[4096];
        bool stdout_open = true, stderr_open = true;
        
        while (stdout_open || stderr_open) {
            FD_ZERO(&read_fds);
            int max_fd = -1;
            
            if (stdout_open) {
                FD_SET(stdout_pipe[0], &read_fds);
                max_fd = std::max(max_fd, stdout_pipe[0]);
            }
            if (stderr_open && config_.capture_stderr) {
                FD_SET(stderr_pipe[0], &read_fds);
                max_fd = std::max(max_fd, stderr_pipe[0]);
            }
            
            if (max_fd == -1) break;
            
            // Set timeout
            timeout_val.tv_sec = config_.timeout.count() / 1000;
            timeout_val.tv_usec = (config_.timeout.count() % 1000) * 1000;
            
            int select_result = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout_val);
            
            if (select_result == -1) {
                break;
            } else if (select_result == 0) {
                result.timed_out = true;
                kill(pid, SIGKILL);
                break;
            }
            
            // Read from stdout
            if (stdout_open && FD_ISSET(stdout_pipe[0], &read_fds)) {
                ssize_t bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    result.stdout_output += std::string(buffer, bytes_read);
                } else {
                    stdout_open = false;
                }
            }
            
            // Read from stderr
            if (stderr_open && config_.capture_stderr && FD_ISSET(stderr_pipe[0], &read_fds)) {
                ssize_t bytes_read = read(stderr_pipe[0], buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    result.stderr_output += std::string(buffer, bytes_read);
                } else {
                    stderr_open = false;
                }
            }
        }
        
        // Close remaining pipes
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        
        // Wait for child process
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            result.exit_code = 128 + WTERMSIG(status);
        }
        
        auto end_time = std::chrono::steady_clock::now();
        result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time
        );
    }
    
    return result;
}

ExecutionResult AsmTestRunner::execute_with_strace(
    std::span<const std::string> args,
    const std::optional<std::string>& stdin_data
) const {
    
    // Build strace command
    std::vector<std::string> strace_args;
    strace_args.push_back("strace");
    
    // Add strace options
    for (const auto& option : config_.strace_options) {
        strace_args.push_back(option);
    }
    
    // Add the executable path
    strace_args.push_back(executable_path_.string());
    
    // Add original arguments
    strace_args.insert(strace_args.end(), args.begin(), args.end());
    
    ExecutionResult result;
    auto start_time = std::chrono::steady_clock::now();
    
    // Create pipes
    int stdout_pipe[2], stderr_pipe[2], stdin_pipe[2];
    
    if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1 || pipe(stdin_pipe) == -1) {
        throw std::runtime_error("Failed to create pipes for strace execution");
    }
    
    // Prepare arguments for execv
    std::vector<char*> exec_args;
    exec_args.reserve(strace_args.size() + 1);
    
    for (const auto& arg : strace_args) {
        exec_args.push_back(const_cast<char*>(arg.c_str()));
    }
    exec_args.push_back(nullptr);
    
    pid_t pid = fork();
    
    if (pid == -1) {
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        throw std::runtime_error("Fork failed for strace execution");
    }
    
    if (pid == 0) {
        // Child process
        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        dup2(stdin_pipe[0], STDIN_FILENO);
        
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        close(stderr_pipe[0]); close(stderr_pipe[1]);
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        
        if (config_.working_directory != std::filesystem::current_path()) {
            if (chdir(config_.working_directory.c_str()) != 0) {
                perror("chdir");
                _exit(127);
            }
        }
        
        execvp("strace", exec_args.data());
        perror("execvp strace");
        _exit(127);
    } else {
        // Parent process - same logic as execute_process
        close(stdout_pipe[1]);
        close(stderr_pipe[1]);
        close(stdin_pipe[0]);
        
        if (stdin_data.has_value() && !stdin_data->empty()) {
            ssize_t written = write(stdin_pipe[1], stdin_data->c_str(), stdin_data->size());
            (void)written;
        }
        close(stdin_pipe[1]);
        
        // Read output with timeout
        char buffer[4096];
        bool stdout_open = true, stderr_open = true;
        fd_set read_fds;
        struct timeval timeout_val;
        
        while (stdout_open || stderr_open) {
            FD_ZERO(&read_fds);
            int max_fd = -1;
            
            if (stdout_open) {
                FD_SET(stdout_pipe[0], &read_fds);
                max_fd = std::max(max_fd, stdout_pipe[0]);
            }
            if (stderr_open) {
                FD_SET(stderr_pipe[0], &read_fds);
                max_fd = std::max(max_fd, stderr_pipe[0]);
            }
            
            if (max_fd == -1) break;
            
            timeout_val.tv_sec = config_.timeout.count() / 1000;
            timeout_val.tv_usec = (config_.timeout.count() % 1000) * 1000;
            
            int select_result = select(max_fd + 1, &read_fds, nullptr, nullptr, &timeout_val);
            
            if (select_result <= 0) {
                if (select_result == 0) result.timed_out = true;
                kill(pid, SIGKILL);
                break;
            }
            
            if (stdout_open && FD_ISSET(stdout_pipe[0], &read_fds)) {
                ssize_t bytes_read = read(stdout_pipe[0], buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    result.stdout_output += std::string(buffer, bytes_read);
                } else {
                    stdout_open = false;
                }
            }
            
            if (stderr_open && FD_ISSET(stderr_pipe[0], &read_fds)) {
                ssize_t bytes_read = read(stderr_pipe[0], buffer, sizeof(buffer) - 1);
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    result.stderr_output += std::string(buffer, bytes_read);
                } else {
                    stderr_open = false;
                }
            }
        }
        
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status)) {
            result.exit_code = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            result.exit_code = 128 + WTERMSIG(status);
        }
        
        auto end_time = std::chrono::steady_clock::now();
        result.execution_time = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time
        );
    }
    
    return result;
}

ExecutionResult AsmTestRunner::run_test(const TestInput& input) const {
    return execute_process(input.args(), input.stdin_data());
}

void AsmTestRunner::assert_output(const TestInput& input, const ExpectedOutput& expected) const {
    auto result = run_test(input);
    
    if (!expected.matches(result)) {
        std::ostringstream error_msg;
        error_msg << std::format("Assembly test failed for executable: {}\n", executable_path_.string())
                  << std::format("Syntax: {}\n", get_syntax_string())
                  << "Arguments: ";
        
        // Join arguments
        bool first = true;
        for (const auto& arg : input.args()) {
            if (!first) error_msg << " ";
            error_msg << arg;
            first = false;
        }
        
        error_msg << std::format("\nExecution time: {}ms\n", result.execution_time.count())
                  << expected.get_mismatch_description(result);
        
        FAIL() << error_msg.str();
    }
}

bool AsmTestRunner::executable_exists() const noexcept {
    return std::filesystem::exists(executable_path_) && 
           std::filesystem::is_regular_file(executable_path_);
}

std::string AsmTestRunner::get_syntax_string() const noexcept {
    switch (syntax_) {
        case AsmSyntax::Intel: return "Intel";
        case AsmSyntax::ATT: return "AT&T";
        default: return "Unknown";
    }
}

} // namespace x86_asm_test
