- What happens if a C string is not null-terminated and it is printed out?

Without a null terminator, the program won't know where the string ends, so it'll probably end up printing out random garbage/memory that corresponds to other objects. This will probably show up as a buffer overflow.

- There are at least three major problems with the provided code. Explain what each of them are and how to fix them.

1. Line 2 - The loop runs as long as i is less than sizeof(str). The size of a pointer is 8 bytes. This is a problem because the char array represented by "hello" only has five elements. This can be fixed by replacing sizeof(str) with strlen(str).

2. Line 5 - The print won't work because str doesn't have a null terminator. This can be fixed by assigning \0 to the last element of the char array/string (str[5] = '\0'). This won't work with the program as is, though. We thus encounter our next error:

3. Line 1 - As implied above by the indexing of str, str currently doesn't have enough memory malloc'ed to account for the null terminator. This can be fixed by adding 1 to strlen("hello") before the sum is multiplied by sizeof(char).

4. The string is never freed.