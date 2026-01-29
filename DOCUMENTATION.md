# Documentation

SvgPathTurtle (version 0.7 alpha)

## Contents

#### Quick Start

* [Installation](#installation)
    * [Linux Instructions](#linux-instructions)
    * [Windows Instructions (experimental)](#windows-instructions-experimental)
* [Tutorials](#tutorials)
  * [Tutorial 1](#tutorial-1-square-3-steps)
  * [Tutorial 2](#tutorial-2-grid-of-squares-6-steps)
  * [Tutorial 3](#tutorial-3-adding-rotation-4-steps)
  * [Explore!](#tutorial-4-explore)
  * [Debugging](#debugging)
* [Web Development](#web-development)
  * [Deployment](#deployment)
  * [Embedded SVG](#embedded-svg)

#### Language Reference

* [Syntax](#syntax)
  * [General Notes](#general-notes)
  * [Expressions](#expressions)
  * [Statements](#statements)
  * [Lambda Functions](#lambda-functions)
  * [Imports](#imports)
* [Commands](#commands)
  * [Positioning](#positioning)
  * [Accessing Turtle State](#accessing-turtle-state)
  * [Modifiers](#modifiers)
  * [Drawing](#drawing)
  * [Quadratic Bezier](#quadratic-bezier)
  * [Cubic Bezier](#cubic-bezier)
  * [Tracing right triangle edges](#tracing-right-triangle-edges)
  * [Formatting](#formatting)
* [Library](#library)
  * [Using Shapes](#using-shapes)
    * [Defining Shapes](#defining-shapes)
    * [Placing Shapes](#placing-shapes)
    * [Shape Example](#shape-example)
  * [Utility Shapes](#utility-shapes)
  * [Matrices](#matrices)
    * [Matrix Wrappers](#matrix-wrappers)
    * [Raw Matrix Operations](#raw-matrix-operations)
    * [Experimental Matrices](#experimental-matrices)

# Quick Start

## Installation

> [!NOTE]
> SvgPathTurtle is currently source-only.  Software development tools are required to
> build it.  Also, there's no install package yet.
> Just follow the instructions below to run it.
> (sorry!  this is just my hobby)

### Linux Instructions

> Note: Requires `make` (gnu make), `cmake`, and `g++`

1. Clone the repo (or click on `Tag` in github and download zip or tar.gz).
1. `cd` into the repo directory.
1. Run `make`
1. To test: run `tools/run_tests`

If there are no failures in the tests, then you're done! You should
now have the following two programs in your repo dir:

`svg_path_turtle`
* Runs turtle programs

`tools/svg_path_turtle_compositor`
* [Web Development](#web-development) will tell you when to use this

Next up: Try the [Tutorials](#tutorials)!

### Windows Instructions (experimental)

First, build with Visual Studio.  Here's the steps if you're unfamiliar:

- Clone the repo to a local dir (or click on `Tag` in github and download zip or tar.gz).
- Install `Visual Studio 2026` (Community is fine)
    * Or any Visual Studio with cmake version 4.0 or greater
- Launch VS, click to open a local folder, and then select the git repo folder.
   * VS will automatically run cmake once it's fully started.
   * Wait for cmake to complete (a few seconds)
- Menu: `Build > Build All`
   * Note: you will see lots of warnings due to `/Wall`, but they should all be fine.

To run the tests:

1. Start `git-bash` (comes with `git`).
    * other `bash` installations should also work.
1. cd to the repo dir and run
   `tools/run_tests`

If there are no failures in the tests, then you're done! You should
now have the following two programs in the repo dir:

`svg_path_turtle.exe`
* Runs turtle programs

`tools/svg_path_turtle_compositor`
* [Web Development](#web-development) will tell you when to use this
* Requires `python`

Next up: Try the [Tutorials](#tutorials)!

## Tutorials

## Tutorial 1: Square (3 steps)

> You will create a turtle program to draw a square.

> [!NOTE]
> You must first follow the instructions in [Installation](#installation).

Step 1: Create a text file called `turtle` (in the repo directory) and paste this code in:

```mysql
M 20 20 
for 4 { f 50 r 90 } z # a square
```

<details><summary>Notes</summary>

> `M` moves to an absolute position without drawing (same as SVG)
>
> `f` moves the turtle forward, drawing a line for the given distance
>
> `r` rotates the turtle to the right (value is in degrees)
>
> `z` closes the path (same as SVG)
>
> `#` introduces a comment.  It ends at the end of the line.

</details>

Step 2: Run this command (while in the repo directory):

```
./svg_path_turtle -s turtle > turtle.svg
```

Step 3: Open `turtle.svg` in your web browser.
* You should see a blue square with a black outline

Congratulations!

<details><summary>Web Developers</summary>

> Note: `-s` creates a bare minimum SVG file, for debugging,
> testing, and play.  But this technique does not allow for 
> non-turtle elements in the SVG file.
> See [Web Development](#web-development) for how to work with full SVG files,
> or even SVG embedded (inlined) within HTML.

</details>

## Tutorial 2: Grid of squares (6 steps)

> You will modify your program to place squares in a grid.

> [!NOTE]
> You must first complete [Tutorial 1](#tutorial-1-square-3-steps)

Step 1: Remove the `M 20 20` command.

Step 2: Add this line at the top of your file:


```mysql
import 'library.svgt'
```

Step 3: Wrap your for-loop in a user defined `square` function, so that you
can call it again and again.  Like so:


```mysql
def square()
{ 
  for 4 { f 50 r 90 } z # a square
}
```

Step 4: Add nested for-loops at the end of the file, to `stamp` a grid of squares:


```mysql
for x = 0..5
  for y = 0..5
  {
    M (40 + x*60) (40 + y*60)
    stamp square
  }

```

<details><summary>Final program</summary>

```mysql
import 'library.svgt'

def square()
{ 
  for 4 { f 50 r 90 } z # a square
}

# make a grid of squares

for x = 0..5
  for y = 0..5
  {
    M (40 + x*60) (40 + y*60)
    stamp square
  }

```

</details>

Step 5: Compile again:


```
./svg_path_turtle -s turtle > turtle.svg
```

Step 6: Refresh `turtle.svg` in your web browser.
* You should see a 6 x 6 grid of blue squares

Done!

## Tutorial 3: Adding Rotation (4 steps)

> You will modify your program to rotate the squares in the grid.

> [!NOTE]
> You must first complete [Tutorial 2](#tutorial-2-grid-of-squares-6-steps)

Step 1: Add `push` and `pop` around the `stamp` line, like so:

```mysql
push stamp square pop
```

These commands save and restore the turtle state (x, y, and direction).

Step 2: Add rotation based on the grid location, by inserting an `r` command
(for "turn right" or "rotate") after `push`.  The angle is calculated off of `x`
and `y` so that it will be different for each square.  Like so:

```mysql
push r (x * y) stamp square pop
```

<details><summary>Final program</summary>

```mysql
import 'library.svgt'

def square()
{ 
  for 4 { f 50 r 90 } z # a square
}

# make a grid of squares

for x = 0..5
  for y = 0..5
  {
    M (40 + x*60) (40 + y*60)
    push r (x * y) stamp square pop
  }

```

</details>

Step 3: Compile again:


```
./svg_path_turtle -s turtle > turtle.svg
```

Step 4: Refresh `turtle.svg` in your web browser.
* You should see your squares trying to fall.  Sort of.

Done!

> For more fun, try removing `push` and `pop`, so that the rotations are
> accumulative.  Then you don't have to do a calculation to make them all
> different.  Like so:
> 
> ```mysql
> r 1 stamp square
> ```
> 
> Each successive square will be rotated 1 more degree.

> [!TIP]
> You will get unexpected results with the previous example if you use a larger
> angle, such as `r 15`.  This is because `square` is not
> currently centering the square around the origin point, and so 
> it unexpectedly rotates around its top-left corner.
>
> To make it rotate around its center, add `M -25 -25` before the `for` loop:
>
> ```mysql
> def square()
> { 
>   M -25 -25
>   for 4 { f 50 r 90 } z # a square
> }
> 
> ```
>
> In general, when writing commands for any kind of shape, always consider around what point the
> object should rotate. 

## Tutorial 4: Explore!

* Check out the [README](README.md) for
  examples of what kinds of drawings you can make.

* Read [Language Syntax](#syntax) and more, to learn about all the
  features.

* Check out the [Library](#library) reference for advanced commands.

* If your program has errors, or does not work properly, see 
  [Debugging](#debugging).

### Debugging

If your file parses properly, but has errors when running, then use `--debug`:


```
./svg_path_turtle --debug turtle_file
```

This will report the line number where the error occurs.

#### Incorrect output

If your program runs without errors but the output is not what you
wanted, here are some additional steps to try:

* If your drawing is larger than 500 x 500, then use the advanced option
  `--svg-out` instead of `-s`.
  * You can specify a larger viewbox.
  * You can also specify some colors, and stroke width and such.
  * Use `--help` for more info.
* If your code fails during execution, you may see a backtrace.  This
  can help with really complicated programs.
* `--trace` outputs a (possibly very large) step-by-step trace of execution,
  which can be loaded into an IDE for easier debugging.
  * Add a second `--trace` option to also output the current state of the turtle
    at each step.
* If your turtle code runs successfully but produces incorrect output, 
  add `--prettyprint` to make the output easier to read (redundant if also
  using `--trace`).

Use `--help` to learn about more options.

## Web Development

Contents:
* [Deployment](#deployment)
* [Embedded SVG](#embedded-svg)

### Deployment

For true web development, you will want to use your own SVG file, perhaps with
many additional elements or even javascript.  This means you must *composite*
your turtle output into that file.

`svg_path_turtle_compositor`
(python & minidom) does this. It also verifies that what is composited
is valid SVG path data.

> [!TIP]
> You are free to use other compositor programs instead.

#### 3 Steps to Composited SVG

Step 1: Add a `src` attribute (yes, this is non-standard) to a `path` element
in your SVG file.  The value for `src` should be a filename that will
hold the output of one of your turtle programs.  Like so:

```svg
<svg ...>
  ...
  <path src="turtle_output.path" />
  ...
</svg>
```

<details><summary>Notes</summary>

> * You can use another attribute name, such as `turtle_src`.  The `-a` option
>   of the compositor allows this.
>
> * You can use any file extension.  `.path` is just an example.
>
> * You can have many such path elements in a single file.
>
> * You can also have placeholder `d` attributes on those path elements.  The
>   compositor will overwrite them in the final SVG file.
>
> * The `src` attribute does not remain after compositing.
> 
> * An error will be shown if the `src` file contains invalid SVG path data

</details>

Step 2: Run your turtle program (without `-s`) to create the path data file.

```
./svg_path_turtle my_turtle_program turtle_output.path
```

Step 3: Now run the compositor on your SVG file:

```
tools/svg_path_turtle_compositor in.svg out.svg
```

You're done! Open `out.svg` in your browser.

> [!TIP]
> For a complex example, see the files in the `icon/` subdir, and the 
> [Icon README](icon/README.md).

### Embedded SVG

`svg_path_turtle_compositor` seems to work fine on HTML.
Like so:

```
tools/svg_path_turtle_compositor in.html out.html
```

This only works for truly embedded SVG.  For `img` tags that reference an SVG
file as their `src`, run the compositor on the SVG file instead.

> [!TIP]
> If you work with other kinds of files that also have a `path` element that has
> `src` attributes, then use the `-a` option to change the expected source attribute
> name to something that won't collide, such as `-a turtle_src`.

# Language

## Contents

* [Syntax](#syntax)
* [Commands](#commands)
* [Library](#library)
* [Matrices](#matrices)

## Syntax

## General Notes

### Declarative Programming Language

SvgPathTurtle is a "declarative" programming language, which mostly means there
is no modifiable state.  All values are constants.  I find this keeps the
descriptions of drawings a bit cleaner.

### Parentheses

Parentheses must be used when the expression is more than a simple number.
Like so: `M -10 (x + 20)`

`-10` is ok.

`x + 20` must be wrapped: `(x + 20)`.

<details><summary>Why?</summary>

> I wanted it to look as much like SVG as possible.  Also, `r 45` is easier
> to type than `r(45)`.
> 
> I'll admit, it can seem odd at times:
>
> `M 10-20` is really `M 10 -20`

</details>

### Commas

`svg_path_turtle` does not accept commas.  Use spaces.

### Uppercase Z

`svg_path_turtle` does not recognize `Z` (uppercase) as a command.  Use
`z` (lowercase) instead.

## Expressions

Numbers: `1` `3.1415` `-1e10`

> All are 64-bit floating point values.

Math: `+x` `-x` `+` `-` `*` `/` `**`

- `x**y` is x to the power of y

Subexpression: `(expression)`

Conditional: `&&` `||` `a ? b : c`

- As in many languages, `&&` and `||` return "the value that made them true", or 0.0.

Comparisons: `>` `<` `>=` `<=`

Equality: `==` `!=` `!x`

- Warning!  Comparing floating point values for equality is only reliable with exact integers (and certain
    fractional values too numerous to list here).

## Statements

```mysql
f 100
  # execute a command

f (300 / 7)
  # expressions require parentheses

# comments look like this!

PI = 3.14159
  # name a value

to_radians = (PI / 180)
  # use an expression instead

## User defined commands

def foo(a b c) { ... }
  # define a command

foo -1 (2/3) PI
  # execute it

## Conditional: if..else if..else

if value  { ... } else { ... }
if (expr) { ... } else { ... }
if x {..} else if y {..} else {..} # if/else-if/else
if value f 100 else f 200
  # braces not required for single statements

## Repetition (loops)

for 1..2..10 { ... }
  # start, step, end (inclusive)

for 1..10 { ... }
  # step defaults to 1

for 10..1 { ... }
  # count down instead of up

for 10 { ... }
  # repeat 10 times

for v = 1..10 { f v }
  # name the loop variable
  # note: the name is only available inside the body

```

> [!NOTE]
> Functions cannot return values in SvgPathTurtle.  This is intentional.
> "The turtle way" is geometry, not math.
>
> Anyway, I thought it would be fun to see what kind of language that results in.

## Lambda Functions

Function names (and anonymous functions) can be passed 
as parameters to other functions that
will use them.  See `library.svgt` for lots of examples.

```mysql
# Define a function that wants a function

def run(some_fn(a b))
{
  some_fn 100 300
}

run M
  # You can pass a builtin (this does "M 100 300")

run my_function
  # Or a user-defined function

run { f 100 r 45 }
  # Or an anonymous function

run {=>(x y) M x y }
  # Anonymous that uses the arguments

run {=>(x) M x x }
  # You can ignore some arguments

run {=>(x y z) ... } # Error!
  # But you can't ask for too many

```

## Imports

`import 'filename'`
* use definitions from another file

`import 'library.svgt'`
* Use the definitions from the included SVGPathTurtle library

## Commands

### Positioning

`m dx dy`  `M x y`
* Move relative, move absolute

`r angle`  `l angle`
* Rotate right or left (degrees)

`d angle`
* Rotate to a specific direction
* Turtle starts at 0 (East).  90 is South.

`aim dx dy`
* Turn to face the given point (relative).
* `aim 0 0` does nothing (the angle would be undefined).

`j distance`
* Jump forward without drawing

`hop`
* Jump in place
* Begins a new path without moving
* Requires `import 'library.svgt'`

### Accessing Turtle State

`turtle.x` - the x position

`turtle.y` - the y position

`turtle.dir` - the direction (degrees)
* Zero is East, 90 is South.

`unique` - returns the next integer each time it is called
* This can be useful for complex programs
* Returns 1 the first time, and then 2, 3, ...

### Modifiers

`up` `down`
* Raise the pen from the paper to stop drawing, and then lower it back down.
  Multiple up's require multiple down's.

`push` `pop`
* Save the current position, angle, and pen height on a stack, and then restore
  it later.

`push_matrix` `pop_matrix`
  * For full info, see [Matrices](#matrices) below.
  * Save the current transform matrix on a stack, and then pop it later.
  * All matrices on the stack are applied when drawing, allowing for nested
    viewports.

### Drawing

`f distance` - Forward
* Draw a line of the given length

`a radius angle` - Arc
* Draw an arc of a circle of the given radius, where the
  arc covers the given angle.  Note: as with normal SVG, to draw a circle
  you must use 180 twice: `a 10 180 a 10 180`.

`orbit x y angle` - Arc around a point
* Draw an arc of a circle centered at the given absolute x and y, that contains
  the current turtle position.

`z` - Close the path

`to dx dy` - Line To
* Draw a line to a given point (relative)
* Requires `import 'library.svgt'`

`ellipse rx ry`
* Draw a complete ellipse with the given two axes, centered around
    the current position.
* The `rx` axis is aligned in the direction the turtle is facing.
* Best used with functions like `stamp` (see [Placing Shapes](#placing-shapes)).
* Note: won't draw properly in the `shear` transformation (SVG cannot distort
        the curves).

[coming soon?]`e rx ry angle1 angle2`
  * [not yet implemented] Draw an arc of an ellipse, tangent to the line the turtle is currently on.
    This would invoke the SVG `a` command.

### Quadratic Bezier

`q dx dy angle`
* Draw a quadratic bezier curve to the given point (relative),
  arriving at the given angle.  The control point is calculated as the
  intersection between the line the turtle was on, and the line defined by
  these arguments.  Parallel lines are detected and result in an error.

`Q x y angle`
* Same as `q`, but the x and y are absolute.

`t distance`
* Smoothly continue a quadratic bezier to the point reached by
  moving forward the given distance.  See SVG docs for `t` for more info.
* Note: to use `t`, you must rotate the turtle in the direction of the
  destination point.  However, the line will nonetheless smoothly continue from the previous
  `q`, `Q`, or `t`.

### Cubic Bezier

`c length1 angle1 length2 angle2 dx dy`
  * Draw a cubic bezier curve to the given final point
    (relative).  The first control point is the end of a line of length `length1`
    extending from the starting point at angle `angle1`.  The second control
    point is the *start* of a line of length `length2` that *arrives* at the final
    point at angle `angle2`.

`C length1 angle1 length2 angle2 x y` 
  * Like `c` but with absolute x and y

`s length2 angle2 dx dy` 
  * Smoothly continue a cubic bezier curve to the
    given final point (relative).  See SVG docs for `s` for more info.

`S length2 angle2 x y`
  * Like `s` but with absolute x and y

### Tracing right triangle edges

These commands might seem odd, but they allow you to draw lengths 
that would
otherwise be impossible to calculate in SvgPathTurtle.

`ah angle hypotenuse`
* "adjacent for hypotenuse": draw the side adjacent to
  the given angle, for a right triangle with a hypotenuse of the given length.

`ao angle opposite`
* "adjacent for opposite": draw the side adjacent to
  the given angle, for a right triangle with an opposite side of the given length.

`ha angle adjacent`
* "hypotenuse for adjacent": draw the hypotenuse for
  a right triangle with the given angle and adjacent side of the given length.

`ho angle opposite`
* "hypotenuse for opposite": draw the hypotenuse for
  a right triangle with the given angle and opposite side of the given length.

`hb adjacent opposite`
* "hypotenuse for both": draw the hypotenuse for
  a right triangle with the given two sides.  This is just the pythagorean
  theorem.

### Formatting

Note: user-defined formatting is generally only output when doing
[Debugging](#debugging).  See the link for more info.

`nl` - emit a newline into the output

`sp` - emit a space into the output

## Library

### Using Shapes

A "shape" is simply a command function (like `square` in the
[Tutorials](#tutorials)) that is intended to be placed using 
placement functions like `stamp`.

#### Defining Shapes

`stamp` will call any function, but it's best if your shape functions adhere to 
these guidelines:

* Assume the turtle is at 0,0, and direction is also zero (East).  Orient your
  drawing so that it makes sense when placed that way.
* Choose your rotation point carefully!  Shapes rotate around 0,0 (the origin).  If
    drawing a square, consider centering it around that point. Other
    shapes may prefer to rotate around a different point (like `arrow`, which
    rotates around the butt end of the arrow shaft).
* The origin also chooses exactly where the shape will land.  The origin point
    of the shape will land exactly on the current turtle point.
* Consider defining your shape with no size argument.  `stomp` can be used to
    enlarge or shrink it.

#### Placing Shapes

> Requires `import 'library.svgt'`

<details><summary>Note about scenes:</summary>

> These placement functions can be nested.  You can build up whole
> scenes by writing shape functions that `stamp` shapes within them, and
> then `stamp` the whole scene at once.
> 
> * See [`tests/multi_scene`](tests/multi_scene) for an example.

</details>

`stamp function`
* Place a shape at the turtle's position and orientation.
* The function should be written according to the guidelines in
    [Defining Shapes](#defining-shapes)
* If the function requires arguments, use an anonymous lambda, like so:
  ```mysql
  stamp { my_shape 30 70 }
  ```

`stomp ratio function`
* Like `stamp`, but make the drawing larger (or smaller)

`place x y dir ratio function`
* Like `stomp`, but at a different position and orientation.
* Turtle does not move.

`mirror function`
* Flips a shape horizontally
* Example use: `stamp { mirror my_shape }`

`flip function`
* Flips a shape vertically
* Example use: `stamp { flip my_shape }`

`smear scalex scaley shape_fn`
* Experimental. Similar to `stomp`, this places an *asymmetrically* scaled shape.

> [!WARNING]
> `smear` won't work well with curves.  SvgPathTurtle can modify the positions of
> the points, but it cannot cause SVG to distort the curves.

#### Shape Example

```mysql
  import 'library.svgt'

  def dot() # a small circle
  {
    M 0 -5 # centers the circle around the origin
    for 2 { a 5 180 } z
  }

  # Place a dot at 10,10

  M 10 10 stamp dot

  # Place 5 more, to the right

  for 5 { j 20 stamp dot }
```

### Utility Shapes

> Requires `import 'library.svgt'`

`circle radius`
* A circle centered around the current point.

`line length`
* A line segment as a complete path.
* It will have proper linecaps.

`arrow` `X` `O`
* Various shapes useful for debugging

## Matrices

### Matrix Wrappers

> Requires `import 'library.svgt'`

These low-level wrappers are provided for experimentation.
They execute a shape function within a specific, temporary
adjustment to the transformation matrix.

> [!TIP]
> For easy and reliable results, use shapes instead
>   (see [Using Shapes](#using-shapes)).

`scale x y shape_fn`

`rotate degrees shape_fn`

`translate x y shape_fn`

`reflect x y shape_fn`

`shear x y shape_fn`

> Note: `shear` does not work well with curves (SVG cannot distort the
    curves).

### Raw Matrix Operations

These underlying operations might be fun to play with, but may give
confusing results.  Matrix multiplication is non-intuitive.

The current matrix is not saved, and the modification is not
temporary.

> Again, consider using shapes instead (see [Using Shapes](#using-shapes)).
> Or at least the [Matrix Wrappers](#transform-operations-matrices).

`rotation angle`

`scaling x y`

`translation dx dy`

`reflection x y`

`shearing x y`

> Note: `shearing`, like `shear`, does not work well with curves (SVG cannot distort the
    curves).

