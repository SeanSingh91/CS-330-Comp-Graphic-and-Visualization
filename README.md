# CS-330-Computational Graphics and Visualization
**Developer:** Sean Singh
**Date:**  August 2026
|--------------------------------------------------------------

### Final Project: Desk Scene
It contains the full Visual Studio solution (source, shaders, and textures) for a 3D scene built from a photo of my own desk, a stack of 
six books, a coffee mug, and a small stone paperweight, along with the Design Decisions document explaining the reasoning behind the geometry, 
textures, lighting, and navigation controls.

|-------------------------------------------------------------

### Reflection

**How do I approach designing software?**

Before this project, I mostly designed by starting with whatever felt easiest and hoping the rest would fall into place. This scene forced a 
different habit: I built the hardest, most varied piece first. The six-book stack has more geometric variety than anything else on the desk; 
each cover is a slightly different width, height, and rotation, and three of the books are three stacked primitives apiece (a top cover, a page 
block, a bottom cover) rather than a single box. Getting that stack to look like it had been set down, rather than a uniform pile, meant thinking 
through overlap and rotation before touching the mug or the paperweight at all. The mug and paperweight came later, once I realized the scene
would only be using two primitive types (box and plane) if I stopped there, and each addition had to justify itself with its own texture and 
shading rather than reusing what already existed. Going forward, I want to keep using that order: build the piece that's most likely to expose a 
structural problem first, then add supporting pieces once the hard part is proven out.

**How do I approach developing programs?**

The biggest shift in how I write code came from the mug. Instead of writing its body, handle, and coffee surface as three separate inline draw 
calls the way the books are handled, I wrote a single `RenderMug()` function that takes a position, a Y-axis rotation, and a scale, and issues 
every draw call the mug needs internally. The handle's offset from the body is computed with the cosine and sine of the rotation angle instead 
of a fixed number, so rotating the mug turns the handle with it instead of leaving it stuck in place. That one function taught me more about 
parameterizing code than anything else in the project. Iteration mattered just as much as writing the function itself; nearly every material and 
lighting value in the scene (the key light's position, the fill light's intensity, how glossy the ceramic looks versus the paperback covers) came 
from running the program, nudging a number, and rebuilding, not from calculating it up front. My approach also had to change as the object count 
grew: earlier milestones got by with three textures and inline values, but by the final project I was managing twelve textures and two point 
lights, and copy-pasting values stopped being sustainable; everything had to route through tagged materials and reusable helpers instead.

**How can computer science help me in reaching my goals?**

Working through the lighting and material setup on this scene, figuring out why the fill light needed to be dimmer and cooler than the key 
light so the far side of the book stack didn't fall into total black, or why leather needed a tighter specular highlight than cloth, gave me a 
much more concrete sense of how light and geometry actually interact, which is knowledge I expect to keep using in any course that touches 
simulation, visualization, or graphics. Professionally, this project pushed my C++ comfort further than any assignment so far, particularly 
around debugging problems that never throw an error but just look wrong on screen, which is its own kind of troubleshooting skill. It also 
showed me the value of structuring a codebase around managers and reusable functions from the start, since the scene only stayed manageable 
because I refactored toward that pattern before it became necessary rather than after. That's a habit I plan to carry into any future work 
involving real-time rendering, tooling, or anything else where the code needs to keep scaling without turning into a pile of one-off values.
