import sys

print("Content-Type: text/plain")
print("")
data = sys.stdin.read()
print("Received data:")
print(data + " from cgi")
