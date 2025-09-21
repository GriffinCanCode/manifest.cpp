#!/bin/bash
# Format and check code quality script for Manifest Engine
# Requires: clang-format, clang-tidy

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Configuration
FORMAT_EXTENSIONS=("*.cpp" "*.hpp" "*.cxx" "*.hxx" "*.cc" "*.c" "*.h")
TIDY_EXTENSIONS=("cpp" "hpp" "cxx" "hxx" "cc" "c" "h")
BUILD_DIR="${PROJECT_ROOT}/build"

# Help function
show_help() {
    cat << EOF
Usage: $0 [OPTIONS]

Modern C++23 code formatting and quality checker for Manifest Engine

OPTIONS:
    -f, --format        Format code using clang-format (modifies files)
    -c, --check         Check formatting (dry-run, no modifications)
    -t, --tidy          Run clang-tidy static analysis
    -a, --all           Run both formatting and static analysis
    -p, --project       Format entire project (default: changed files only)
    -h, --help          Show this help message
    
EXAMPLES:
    $0 --check          # Check if code is properly formatted
    $0 --format         # Format changed files
    $0 --all --project  # Format entire project and run static analysis
EOF
}

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if required tools are available
check_prerequisites() {
    local missing_tools=()
    
    if ! command -v clang-format >/dev/null 2>&1; then
        missing_tools+=("clang-format")
    fi
    
    if ! command -v clang-tidy >/dev/null 2>&1; then
        missing_tools+=("clang-tidy")
    fi
    
    if [ ${#missing_tools[@]} -ne 0 ]; then
        log_error "Missing required tools: ${missing_tools[*]}"
        log_info "Install with: brew install llvm (macOS) or apt install clang-tools (Ubuntu)"
        exit 1
    fi
    
    # Check tool versions
    local clang_format_version
    clang_format_version=$(clang-format --version | grep -oE '[0-9]+\.[0-9]+' | head -1)
    log_info "Using clang-format version $clang_format_version"
    
    local clang_tidy_version
    clang_tidy_version=$(clang-tidy --version | grep -oE '[0-9]+\.[0-9]+' | head -1)
    log_info "Using clang-tidy version $clang_tidy_version"
}

# Find source files
find_source_files() {
    local project_scope="$1"
    local files=()
    
    if [[ "$project_scope" == "true" ]]; then
        log_info "Finding all C++ source files in project..."
        while IFS= read -r -d '' file; do
            files+=("$file")
        done < <(find "$PROJECT_ROOT/src" -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.cxx" -o -name "*.hxx" -o -name "*.cc" -o -name "*.c" -o -name "*.h" \) -print0)
    else
        log_info "Finding changed C++ source files..."
        # Get changed files from git
        while IFS= read -r file; do
            if [[ -f "$file" ]] && [[ "$file" =~ \.(cpp|hpp|cxx|hxx|cc|c|h)$ ]]; then
                files+=("$file")
            fi
        done < <(git diff --name-only HEAD~1 2>/dev/null || echo "")
        
        # If no git changes found, fall back to all files
        if [ ${#files[@]} -eq 0 ]; then
            log_warning "No git changes found, scanning entire project"
            while IFS= read -r -d '' file; do
                files+=("$file")
            done < <(find "$PROJECT_ROOT/src" -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.cxx" -o -name "*.hxx" -o -name "*.cc" -o -name "*.c" -o -name "*.h" \) -print0)
        fi
    fi
    
    if [ ${#files[@]} -eq 0 ]; then
        log_warning "No C++ source files found"
        return 1
    fi
    
    log_info "Found ${#files[@]} source files to process"
    printf '%s\n' "${files[@]}"
}

# Format code using clang-format
format_code() {
    local check_only="$1"
    local project_scope="$2"
    local files_to_process
    
    log_info "Starting code formatting (check_only: $check_only)..."
    
    # Use while read loop instead of mapfile for better compatibility
    files_to_process=()
    while IFS= read -r file; do
        files_to_process+=("$file")
    done < <(find_source_files "$project_scope")
    
    if [ ${#files_to_process[@]} -eq 0 ]; then
        return 0
    fi
    
    local issues_found=0
    local format_args=()
    
    if [[ "$check_only" == "true" ]]; then
        format_args+=("--dry-run" "--Werror")
        log_info "Checking code formatting (dry-run mode)..."
    else
        format_args+=("-i")
        log_info "Formatting code (modifying files)..."
    fi
    
    # Process files in batches for better performance
    local batch_size=50
    for ((i = 0; i < ${#files_to_process[@]}; i += batch_size)); do
        local batch=("${files_to_process[@]:i:batch_size}")
        
        if ! clang-format "${format_args[@]}" "${batch[@]}" 2>/dev/null; then
            ((issues_found++))
            if [[ "$check_only" == "true" ]]; then
                log_warning "Formatting issues found in batch starting at index $i"
                # Show specific files with issues
                for file in "${batch[@]}"; do
                    if ! clang-format --dry-run --Werror "$file" >/dev/null 2>&1; then
                        log_warning "  - $file"
                    fi
                done
            fi
        fi
    done
    
    if [[ "$check_only" == "true" ]]; then
        if [ $issues_found -eq 0 ]; then
            log_success "All files are properly formatted"
            return 0
        else
            log_error "Found formatting issues in $issues_found batch(es)"
            log_info "Run '$0 --format' to fix formatting issues"
            return 1
        fi
    else
        log_success "Code formatting completed"
        return 0
    fi
}

# Run clang-tidy static analysis
run_static_analysis() {
    local project_scope="$1"
    local files_to_process
    
    log_info "Starting static analysis with clang-tidy..."
    
    # Use while read loop instead of mapfile for better compatibility
    files_to_process=()
    while IFS= read -r file; do
        files_to_process+=("$file")
    done < <(find_source_files "$project_scope")
    
    if [ ${#files_to_process[@]} -eq 0 ]; then
        return 0
    fi
    
    # Ensure build directory exists and has compile_commands.json
    if [[ ! -f "$BUILD_DIR/compile_commands.json" ]]; then
        log_warning "compile_commands.json not found in $BUILD_DIR"
        log_info "Generating compilation database..."
        
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        
        if ! cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON "$PROJECT_ROOT" >/dev/null 2>&1; then
            log_error "Failed to generate compile_commands.json"
            log_info "Make sure CMake is configured properly"
            return 1
        fi
        
        cd "$PROJECT_ROOT"
    fi
    
    local total_issues=0
    local batch_size=20
    
    # Process files in smaller batches for clang-tidy
    for ((i = 0; i < ${#files_to_process[@]}; i += batch_size)); do
        local batch=("${files_to_process[@]:i:batch_size}")
        
        log_info "Analyzing batch $((i/batch_size + 1))/$(((${#files_to_process[@]} + batch_size - 1) / batch_size))..."
        
        local batch_output
        batch_output=$(clang-tidy -p "$BUILD_DIR" --quiet "${batch[@]}" 2>&1 || true)
        
        if [[ -n "$batch_output" ]]; then
            echo "$batch_output"
            local batch_issues
            batch_issues=$(echo "$batch_output" | grep -c "warning:\|error:" || true)
            ((total_issues += batch_issues))
        fi
    done
    
    if [ $total_issues -eq 0 ]; then
        log_success "No static analysis issues found"
        return 0
    else
        log_warning "Found $total_issues static analysis issues"
        log_info "Review the output above to fix the issues"
        return 1
    fi
}

# Main function
main() {
    local format_mode=""
    local run_tidy=false
    local project_scope=false
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -f|--format)
                format_mode="format"
                shift
                ;;
            -c|--check)
                format_mode="check"
                shift
                ;;
            -t|--tidy)
                run_tidy=true
                shift
                ;;
            -a|--all)
                format_mode="format"
                run_tidy=true
                shift
                ;;
            -p|--project)
                project_scope=true
                shift
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                log_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    # Default to check mode if no options specified
    if [[ -z "$format_mode" && "$run_tidy" == false ]]; then
        format_mode="check"
    fi
    
    log_info "Starting Manifest Engine code quality check..."
    log_info "Project root: $PROJECT_ROOT"
    
    check_prerequisites
    
    local exit_code=0
    
    # Run formatting
    if [[ -n "$format_mode" ]]; then
        local check_only
        [[ "$format_mode" == "check" ]] && check_only="true" || check_only="false"
        
        if ! format_code "$check_only" "$project_scope"; then
            exit_code=1
        fi
    fi
    
    # Run static analysis
    if [[ "$run_tidy" == true ]]; then
        if ! run_static_analysis "$project_scope"; then
            exit_code=1
        fi
    fi
    
    if [ $exit_code -eq 0 ]; then
        log_success "All code quality checks passed!"
    else
        log_error "Some code quality checks failed"
    fi
    
    exit $exit_code
}

# Change to project root directory
cd "$PROJECT_ROOT"

# Run main function with all arguments
main "$@"
