#!/bin/bash

set -e

ROOT="www"

echo "📁 Creating Webserv demo website structure..."

# Directories
mkdir -p \
  $ROOT/errors \
  $ROOT/static \
  $ROOT/redirect \
  $ROOT/autoindex \
  $ROOT/upload \
  $ROOT/cgi-bin \
  $ROOT/files

# Root index
cat > $ROOT/index.html << 'EOF'
<!DOCTYPE html>
<html>
<head><title>Webserv Demo</title></head>
<body>
<h1>Welcome to Webserv</h1>
<ul>
  <li><a href="/static/about.html">Static file</a></li>
  <li><a href="/autoindex/">Autoindex directory</a></li>
  <li><a href="/redirect">Redirection</a></li>
  <li><a href="/upload">File Upload</a></li>
  <li><a href="/cgi-bin/hello.py">CGI Hello</a></li>
</ul>
</body>
</html>
EOF

# Static page
cat > $ROOT/static/about.html << 'EOF'
<h1>About</h1>
<p>This is a static HTML file served by Webserv.</p>
EOF

# Error pages
cat > $ROOT/errors/404.html << 'EOF'
<h1>404 Not Found</h1>
<p>The requested resource does not exist.</p>
EOF

cat > $ROOT/errors/413.html << 'EOF'
<h1>413 Payload Too Large</h1>
<p>Request body exceeded configured limit.</p>
EOF

# Autoindex files
echo "File 1 content" > $ROOT/autoindex/file1.txt
echo "File 2 content" > $ROOT/autoindex/file2.txt

# Upload page
cat > $ROOT/upload/index.html << 'EOF'
<h1>Upload File</h1>
<form action="/upload" method="POST" enctype="multipart/form-data">
  <input type="file" name="file">
  <button type="submit">Upload</button>
</form>
EOF

# CGI: hello.py
cat > $ROOT/cgi-bin/hello.py << 'EOF'
#!/usr/bin/env python3
print("Content-Type: text/html")
print()
print("<h1>Hello from CGI</h1>")
EOF

# CGI: echo.py
cat > $ROOT/cgi-bin/echo.py << 'EOF'
#!/usr/bin/env python3
import os
print("Content-Type: text/plain")
print()
print("REQUEST_METHOD =", os.getenv("REQUEST_METHOD"))
print("QUERY_STRING =", os.getenv("QUERY_STRING"))
EOF

# Make CGI scripts executable
chmod +x $ROOT/cgi-bin/*.py

# Files root
cat > $ROOT/files/index.html << 'EOF'
<h1>Files Root</h1>
<p>This directory is mounted separately.</p>
EOF

echo "✅ Webserv demo website created successfully."
echo "📂 Root directory: $ROOT"

