# Q0
value_size is probably necessary because the allocator needs to know how much space to allocate for the value_t; similarly, when you free the value_t, you also need its size to do so (see: setting the header/footer in the malloc project required you to know how much space the corresponding block_t took up).
# Q1
TOMBSTONE_REF is a type of reference that allows us to avoid rearranging dictionary elements when references are deleted.
# Q2
You still need value_size because you need to know how much space/how many bytes the corresponding value_t took up; this allows us to determine whether we can reuse/re-malloc that space later for other values. The reference count doesn't matter because you don't care about how many references formerly pointed to this now-freed space (since you're probably going to re-use/re-malloc that space later, you can just tell another reference to point to that same space).