with open('src/web_server.cpp', 'r') as f:
    cpp = f.read()

with open('webgui.html', 'r') as f:
    html = f.read()

start_marker = 'const char* html_content = R"====('
end_marker = ')====";'

start_idx = cpp.find(start_marker)
if start_idx != -1:
    start_idx = cpp.find('\n', start_idx) + 1
    end_idx = cpp.find(end_marker, start_idx)
    if end_idx != -1:
        # Move back one to keep the newline before )===="; if needed, actually it doesn't matter much.
        # But let's just make sure we capture it perfectly.
        # Find the actual line of end_marker
        end_idx = cpp.rfind('\n', start_idx, end_idx) + 1
        new_cpp = cpp[:start_idx] + html + "\n" + cpp[end_idx:]
        with open('src/web_server.cpp', 'w') as f:
            f.write(new_cpp)
        print("Updated successfully.")
    else:
        print("End marker not found.")
else:
    print("Start marker not found.")
