# Q1
If a function is re-entrant, then you can run it on multiple threads safely
(without having the threads interfere with each other. strtok() is not
re-entrant because it uses a static buffer while parsing the input string.
In contrast, strtok_r() uses a char pointer to safely maintain context
between calls that parse the same string.