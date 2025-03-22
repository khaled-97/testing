# Changelog

## [1.1.3] - 2025-03-22

### Fixed
- Fixed C90 standard compliance issues:
  - Moved variable declarations to the beginning of code blocks in first_pass.c, code.c, and instructions.c
  - Replaced compound literals with temporary variables in code.c to comply with C90 standards
  - Fixed mixed declarations and code warnings throughout the codebase
  - Ensured all code follows ANSI C90 standards with strict pedantic checks

## [1.1.2] - 2025-03-22

### Added
- Enhanced .data directive validation:
  - Strict number format checking: only digits allowed with optional +/- prefix
  - Improved error messages showing complete invalid tokens (e.g., 'abc123')
  - Clear error message when sign (+/-) is used without a number
  - Improved error detection for malformed numbers
- Improved comma validation in data instructions:
  - Added specific error message for multiple consecutive commas
  - Added specific error message for trailing commas
  - Better distinction between different comma-related errors

### Changed
- Made macro syntax requirements stricter:
  - Macro definition line must only contain "mcro" and the macro name
  - Macro end line must only contain "mcroend"
  - Extra content on these lines will now trigger an error and halt preprocessing

## [1.1.1] - 2025-03-21

### Added
- Added strict error checking for assembly instructions with program termination on errors:
  - Validation for single-operand instructions (clr, not, inc, dec, jmp, bne, jsr, red, prn)
  - Validation for two-operand instructions (mov, cmp, add, sub, lea)
  - Validation for zero-operand instructions (rts, stop)
  - Register validation to ensure only r0-r7 are accepted (halts on invalid registers)
  - Improved immediate value validation (#number) with detailed error messages
- Enhanced error handling system:
  - Shows operation name and number of operands received
  - Displays detailed register error messages with immediate program termination
  - Program halts when invalid register numbers are encountered (e.g., r8)
  - Returns error status to stop further processing on validation failures

## [1.1.0] - 2025-03-21

### Added
- Added preprocessor to handle macros in assembly code
- Created preprocessor.c and preprocessor.h files for macro processing
- Implemented macro expansion functionality
- Added validation for macro names to prevent conflicts with instructions and keywords
- Modified assembler.c to use the preprocessor before processing assembly files
- Updated build process to include the preprocessor

### Changed
- Modified the assembler to read from .am files (preprocessed) instead of .as files
- Updated clean target in Makefile to remove .am files

## [1.0.0] - 2025-03-17

### Fixed
- Fixed file extension handling in assembler.c to properly append .as extension to input filenames
- Removed unused 'dot' variable in assembler.c to fix compiler warning
- Fixed operand processing in second_pass.c to ensure all external references are properly recorded
- Fixed data symbol placement to ensure they appear after code symbols (matching original implementation)

### Changed
- Modified output file formatting in writefiles.c:
  - Changed encoding to use hexadecimal format instead of base-4 encoding
  - Used 7-digit addresses with leading zeros (0000100 instead of 0100) in all output files
  - Applied consistent formatting for .ent and .ext files
- Updated memory layout logic to match original implementation:
  - Code section starts at address 100
  - Data section follows immediately after code section
  - External references are recorded with original symbol names

### Notes
- While the actual machine code encoding differs from the original implementation, the functional behavior is identical
- The assembler now correctly processes input.as files and produces properly formatted output files (.ob, .ext, .ent)
- All external references are correctly recorded in the .ext file
