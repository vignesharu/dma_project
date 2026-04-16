---
description: Configure an Infineon Aurix MCU peripheral using LLD APIs with reference manual guidance. Triggers the full analysis → code generation → review workflow.
agent: infineoncopilot
argument-hint: Describe the peripheral to configure. Include peripheral name, pins, speed/baud rate, mode, and any special requirements. Example: Configure QSPI0 as master at 1 MHz on P15.3/P15.4/P15.5.
tools:
  - agent
  - infineon-copilot/get_lld_catalog
  - infineon-copilot/get_lld_content
  - infineon-copilot/search_user_manual
  - infineon-copilot/get_peripheral_info
  - editFiles
---

Configure the following Infineon Aurix MCU peripheral based on the user's requirements:

$input

Follow the full workflow:

1. **Analyze requirements** — delegate to `@peripheral-analyst`:
   - Search the Infineon Aurix reference manual for register descriptions, clock tree,
     and port pin alternate function mappings.
   - Identify all required configuration settings with manual page citations.

2. **Generate configuration code** — delegate to `@config-developer`:
   - Retrieve exact LLD API signatures from the knowledge catalog.
   - Generate a complete, compilable C configuration file using LLD functions.
   - Include the correct initialization order and inline comments with page refs.

3. **Review the configuration** — delegate to `@config-reviewer`:
   - Cross-check clock enables, alternate functions, and register values.
   - Confirm the initialization sequence is correct.

4. **Deliver the final result** to the user with:
   - The complete configuration C file (ready to copy into the project).
   - A brief explanation of key settings and their manual references.
   - Any assumptions made about clock frequencies or target Aurix variant.
