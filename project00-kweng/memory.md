- If I want to use data after the function it was created in returns, where should I put it? How do I put it there?

The stack is used to store temporary variables, so after the function finishes running, the relevant variables are cleared from the stack. Thus, the data should be stored on the heap via malloc. (Return a pointer from the function.)

- What are the differences between heap and stack memory?

The stack is used to store variables temporarily until the function they were initialized in finishes running. It allocates and frees memory automatically, but variables cannot be dynamically reallocated. The heap stores variables indefinitely after they have been malloc'ed. Variables can be dynamically reallocated, but they must also be manually freed.

- What is a memory leak and how would you fix one?

A memory leak happens when a block of memory that is malloc'ed isn't free. To fix one, free all pointers.

- Determine the smallest number of bytes that will make a single call to malloc fail (i.e., return NULL) on compute-cpu2. Include the number and explain how you determined it in the markdown file.

About 1.34 * 10^11 bytes will make the call to malloc fail. I found it by first starting with a small number of bytes in the malloc call, then adding a 0 every time a call was successful. Then, I lowered each digit (in order of place value) until the malloc call worked before moving on to the next digit and raising it until it failed. I repeated this process until I started getting inconsistent results (malloc would work sometimes and wouldn't work other times with the same number of bytes allocated).