# Styleguide
## Formating of source files
This follows the [LLVM coding standards](https://llvm.org/docs/CodingStandards.html) in general with an indention of 4 spaces. To ensure this, clang-format-11 is used with this `.clang-format` configuration file:

    ---
    # We'll use defaults from the LLVM style, but with 4 columns indentation.
    BasedOnStyle:  LLVM
    IndentWidth: 4
    ---
    Language: Cpp
    # Force pointers to the type for C++.
    PointerAlignment: Left

and use it, for example:

    clang-format-11 upnp/src/api/upnpapi.cpp

Just add option `-i` to modify the source files.

## Git Commit Messages
The [Udacity Git Commit Message Style Guide](https://udacity.github.io/git-styleguide/) served as a template.

### Message Structure
A commit messages consists of three distinct parts separated by a blank line: the title, an optional body and an optional footer. The layout looks like this:

    type: Subject

    body

    footer

The title consists of the type of the message and subject.

### The Type
The type is contained within the title and can be one of these types:

- feat: A new feature
- fix: A bug fix
- docs: Changes to documentation
- style: Formatting, missing semi colons, etc; no code change
- refactor: Refactoring production code
- test: Adding tests, refactoring test; no production code change
- build: Updating build tasks, package manager configs, etc; no production code change

### The Subject
Subjects should be no greater than 50 characters, should begin with a capital letter and do not end with a period.

Use an imperative tone to describe what a commit does, rather than what it did. For example, use change; not changed or changes.

### The Body
Not all commits are complex enough to warrant a body, therefore it is optional and only used when a commit requires a bit of explanation and context. Use the body to explain the what and why of a commit, not the how.

When writing a body, the blank line between the title and the body is required and you should limit the length of each line to no more than 72 characters.

### The Footer
The footer is optional and is used to reference issue tracker IDs.

### Example Commit Message
    feat: Summarize changes in maximal 50 characters

    More detailed explanatory text, if necessary. Wrap it to about 72
    characters or so. In some contexts, the first line is treated as the
    subject of the commit and the rest of the text as the body. The
    blank line separating the summary from the body is critical (unless
    you omit the body entirely); various tools like `log`, `shortlog`
    and `rebase` can get confused if you run the two together.

    Explain the problem that this commit is solving. Focus on why you
    are making this change as opposed to how (the code explains that).
    Are there side effects or other unintuitive consequences of this
    change? Here's the place to explain them.

    Further paragraphs come after blank lines.

     - Bullet points are okay, too

     - Typically a hyphen or asterisk is used for the bullet, preceded
       by a single space, with blank lines in between, but conventions
       vary here

    Put references to an issue at the bottom, like this:

    Resolves: #123
    See also: #456, #789
