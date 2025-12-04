#!/bin/bash
# Bestow System Demos Test Runner
#
# Usage:
#   ./run_tests.sh              # Run all non-interactive tests
#   ./run_tests.sh --all        # Run all tests (requires display)
#   ./run_tests.sh --visual     # Run only visual tests
#   ./run_tests.sh --quick      # Run quick existence checks only
#   ./run_tests.sh <test_file>  # Run specific test file

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Bestow System Demos Test Runner${NC}"
echo "=================================="
echo ""

# Check if pytest is available
if ! command -v pytest &> /dev/null; then
    echo -e "${YELLOW}pytest not found. Installing dependencies...${NC}"
    pip install -r requirements.txt
fi

# Parse arguments
PYTEST_ARGS=""
case "${1:-}" in
    --all)
        echo "Running ALL tests (including interactive)..."
        PYTEST_ARGS="-v"
        ;;
    --visual)
        echo "Running visual tests only..."
        PYTEST_ARGS="-v -m visual"
        ;;
    --interactive)
        echo "Running interactive tests only..."
        PYTEST_ARGS="-v -m interactive"
        ;;
    --quick)
        echo "Running quick existence checks only..."
        PYTEST_ARGS="-v -k 'exists'"
        ;;
    --parallel)
        echo "Running tests in parallel..."
        PYTEST_ARGS="-v -n auto"
        ;;
    "")
        echo "Running non-interactive tests..."
        PYTEST_ARGS="-v -m 'not interactive and not visual'"
        ;;
    *)
        echo "Running specific test: $1"
        PYTEST_ARGS="-v $1"
        ;;
esac

# Run pytest
echo ""
echo -e "${YELLOW}Command: pytest $PYTEST_ARGS${NC}"
echo ""

pytest $PYTEST_ARGS

# Report results
EXIT_CODE=$?
echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
else
    echo -e "${RED}Some tests failed. Exit code: $EXIT_CODE${NC}"
fi

exit $EXIT_CODE
