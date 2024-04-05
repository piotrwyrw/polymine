## Polymine

A transpiled programming language implemented as a side-project of mine

### To-Do
- [ ] Turn the function definition into a proper expression atom (both parser and semantics level)
- [X] Standardise the use of either blocks or compound for storing function parameters, capture variables, .., and lambda types
- [ ] Thus introduce a new data type for function definitions. This type should not only hold the return type of the
  function, but also the types of the individual parameters.
- [ ] On the semantic analysis level: Allow function definitions to be assigned to a variable. Prevent definitions
  from ever constituting a part of a binary expression. Ensure type compatibility in assignments, function calls, etc.
- [ ] Add type inference to variables