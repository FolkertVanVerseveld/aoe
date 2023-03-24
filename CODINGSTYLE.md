# Coding style

*This is still incomplete. Suggestions are certainly welcome*

I (Folkert van Verseveld) have been a fan of the linux kernel coding
style. New code is going to be using the linux kernel coding style. The
current code is a bit of a mess and is undergoing refactoring to also
comply to the coding style.

The points that follow indicate the coding style I want to enforce on
all components that I have to maintain.

Take a look at [this coding style](https://github.com/torvalds/linux/blob/master/Documentation/process/coding-style.rst)
for more details.

## Identation

Only tabs are allowed for indenting blocks to the correct scope. I
expect tabs to be 8 spaces wide and nesting more than three levels is
usually an indicator that the code is a mess anyway and that you should
fix it!

## Documentation

Don't be Captain Obvious and document what it tries to accomplish, not
how.
