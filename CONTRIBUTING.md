# Header and Copyrights

Verification of License and Copyrights is automated on Github by
[Licensing test](https://github.com/intel/ledmon/blob/main/tests/licensing.py).

The rules for Licenses and Copyrights:
- Preferred comment mark should be used for file type. Please prefer to test or other files for
  examples. For `.c` and `.h` files, it is `//`.
- [SPDX Licenses](https://spdx.org/licenses/) must be used:
  - It must be on first  or in third line (only if interpreter is defined);
  - Verification script contains set of allowed licenses for directories to avoid legal issues.
    If you need to honour other license type, test must be extended.
- Immediately after SPDX header Copyright lines may come. There are multiple Copyrights lines
  allowed.
- Only Intel copyright must follow strict style check.
- The block must be ended by empty line.

# Coding

Commits must pass kernel [Checkpatch](https://docs.kernel.org/dev-tools/checkpatch.html). There are
some excludes and it is automatically tested on Github.
