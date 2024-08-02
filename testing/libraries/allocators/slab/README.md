# OMEN'S Slab allocator module

Author: [Xabier Iglesias](mailto://hola@celeiro.gal)

## Dependencies

- A page level allocator (e.g. buddy allocator)
- A panic app

## Description

This module provides a slab allocator for the kernel. It is a general-purpose allocator that can be used to allocate memory for objects of a specific size. It is based on the slab allocator described in the paper "[The Slab Allocator](https://web.archive.org/web/20240701034716/https://people.eecs.berkeley.edu/~kubitron/courses/cs194-24-S14/hand-outs/bonwick_slab.pdf): An Object-Caching Kernel Memory Allocator" by Jeff Bonwick.

The underlying allocator for pmm is the buddy allocator as stated [here](https://www.chudov.com/tmp/LinuxVM/html/understand/node60.html)