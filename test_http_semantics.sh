#!/bin/bash

#############################################################################
#                     HTTP SEMANTICS TEST SUITE                            #
#                  Tests GET/POST/DELETE method handling                   #
#############################################################################

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

PASS=0
FAIL=0

# Function to test HTTP method × resource combination
test_case() {
    local method=$1
    local path=$2
    local expected=$3
    local description=$4
    local data=$5

    # if [$data]; then
    #     result=$(curl -s -w "%{http_code}" -X $method "http://localhost:8080$path" -d "$data" -o /dev/null 2>&1)
    # else
        result=$(curl -s -w "%{http_code}" -X $method "http://localhost:8080$path" -o /dev/null 2>&1)

    if [ "$result" = "$expected" ]; then
        echo -e "${GREEN}✓${NC} $method $path → $result ($description)"
        ((PASS++))
    else
        echo -e "${RED}✗${NC} $method $path → $result (expected $expected) - $description"
        ((FAIL++))
    fi
}

echo "=========================================="
echo "   HTTP SEMANTICS TEST SUITE"
echo "   Testing GET/POST/DELETE method handling"
echo "=========================================="
echo

# ROOT LOCATION TESTS
echo "=== ROOT LOCATION (/) ==="
test_case "GET" "/" "200" "serve root"
test_case "POST" "/" "405" "POST not allowed"
test_case "DELETE" "/" "405" "DELETE not allowed"
echo

# STATIC LOCATION TESTS (with trailing slash)
echo "=== STATIC LOCATION (/static/) ==="
test_case "GET" "/static/" "200" "serve /static/"
test_case "POST" "/static/" "405" "POST not allowed"
test_case "DELETE" "/static/" "405" "DELETE not allowed"
echo

# STATIC FILE TESTS
echo "=== STATIC FILE (/static/about.html) ==="
test_case "GET" "/static/about.html" "200" "GET file"
test_case "POST" "/static/about.html" "405" "POST not allowed"
test_case "DELETE" "/static/about.html" "405" "DELETE not allowed"
echo

# REDIRECT LOCATION TESTS
echo "=== REDIRECT LOCATION (/redirect/) ==="
test_case "GET" "/redirect/" "301" "GET redirect (return directive)"
test_case "POST" "/redirect/" "405" "POST not allowed (safe methods only!)"
test_case "DELETE" "/redirect/" "405" "DELETE not allowed (safe methods only!)"
echo

# AUTOINDEX TESTS
echo "=== AUTOINDEX LOCATION (/autoindex/) ==="
test_case "GET" "/autoindex/" "200" "GET autoindex"
test_case "POST" "/autoindex/" "405" "POST not allowed"
test_case "DELETE" "/autoindex/" "405" "DELETE not allowed"
echo

# UPLOAD TESTS
echo "=== UPLOAD LOCATION (/upload/) ==="
test_case "GET" "/upload/" "200" "serve /upload/"
test_case "POST" "/upload/" "200" "POST allowed (upload_store)"
test_case "DELETE" "/upload/" "405" "DELETE directory not allowed"
echo

# UPLOAD FILE TESTS
echo "=== UPLOAD FILE (/upload/index.html) ==="
test_case "GET" "/upload/index.html" "200" "GET existing file"
test_case "POST" "/upload/" "200" "POST to file allowed"
test_case "DELETE" "/upload/test.txt" "200" "DELETE file allowed"
echo

# CGI TESTS
echo "=== CGI LOCATION (/cgi-bin/) ==="
test_case "GET" "/cgi-bin/hello.py" "200" "GET CGI script"
test_case "POST" "/cgi-bin/hello.py" "405" "POST not allowed (GET only by default)"
test_case "DELETE" "/cgi-bin/hello.py" "405" "DELETE not allowed"
echo

# NONEXISTENT TESTS
echo "=== NONEXISTENT RESOURCES ==="
test_case "GET" "/nonexistent/" "404" "GET nonexistent"
test_case "POST" "/nonexistent/" "405" "POST nonexistent"
test_case "DELETE" "/nonexistent/" "405" "DELETE nonexistent"
echo

# SUMMARY
echo "=========================================="
echo -e "Results: ${GREEN}$PASS PASSED${NC}, ${RED}$FAIL FAILED${NC}"
echo "=========================================="
echo

# Detailed breakdown
TOTAL=$((PASS + FAIL))
PASS_PERCENT=$((PASS * 100 / TOTAL))

echo "Pass rate: $PASS/$TOTAL ($PASS_PERCENT%)"
echo

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}✓ ALL TESTS PASSED!${NC}"
    exit 0
else
    echo -e "${RED}✗ $FAIL test(s) failed${NC}"
    exit 1
fi
