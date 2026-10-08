# Standard UTF-8 strings

Engine text interfaces use `std::string` and `std::string_view`. Text is UTF-8;
standard size, length, indexing, iteration and substrings operate on bytes.
The former Nau string classes and tiny_utf8 dependency have been removed.

This changes source and binary interfaces. Rebuild the engine, modules, tools,
editor and game modules together using compatible toolchain, configuration and
runtime linkage. Install into a fresh SDK directory: installing over an old SDK
can leave retired headers and dependency packages behind. Reconfigure and rebuild
generated projects against that SDK; do not mix pre-migration module DLLs.

| Former operation | Replacement |
| --- | --- |
| `nau::string`, `nau::string_view` | `std::string`, `std::string_view` |
| `.tostring()` | The standard string itself or a borrowed view |
| `nau::string::format(pattern, args...)` | `nau::utils::format(pattern, args...)` in `nau/string/format.h` |
| `.append_format(pattern, args...)` | `value += nau::utils::format(pattern, args...)` |
| Encoding constructors | `nau::strings::toUtf8(input)` in `nau/string/string_conv.h` |
| `.tou16string()`, `.tou32string()` | `utf8ToUtf16(value)`, `utf8ToUtf32(value)` |
| Code-point `.length()`, `.substr()` | `codePointCount(value)`, `codePointSlice(value, position, count)` |
| Code-point mutation/iteration | Decode with `utf8ToUtf32`, edit/iterate, encode with `toUtf8` |
| `_ns` literals | Standard byte literals or explicit `toUtf8(u8"...")` |

Checked conversions validate the complete input and report a diagnostic followed
by an empty result for malformed input, including in exception-disabled Web
builds. Standard string construction stores bytes and does not validate or
transcode. `isValidUtf8` checks bytes without a diagnostic. Length-delimited
conversions retain embedded nulls. The pointer overload of `toUtf8` treats null
external C strings as empty; never construct a standard string/view from a null
pointer.

Code-point helpers count Unicode scalar values, not grapheme clusters. Slicing
accepts an end position, clamps the count and diagnoses positions beyond the end.
Borrowed views require backing storage to remain alive and stable: retain an
owned formatted string before creating a view used beyond the expression.

`hash_string` retains its existing identifier algorithm, hash-only equality,
target word width and first-null termination. Standard container hashes are
transient and are unsuitable for persistent identifiers.
