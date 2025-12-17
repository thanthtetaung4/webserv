#!/usr/bin/env python3
import os
print("Content-Type: text/plain")
print()
print("REQUEST_METHOD =", os.getenv("REQUEST_METHOD"))
print("QUERY_STRING =", os.getenv("QUERY_STRING"))
