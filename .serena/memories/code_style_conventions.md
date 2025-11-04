# Code Style and Conventions

## Naming Conventions
- **Structs**: PascalCase (e.g., `GameState`, `PlayerInfo`, `Card`)
- **Enums**: PascalCase for type, UPPER_CASE for values (e.g., `Rank`, `TWO`, `ACE`)
- **Functions**: snake_case (e.g., `handle_client`, `send_game_state`, `card_to_string`)
- **Variables**: snake_case (e.g., `num_players`, `current_bet`, `client_socket`)
- **Constants**: UPPER_CASE with underscores (e.g., `MAX_PLAYERS`, `BUFFER_SIZE`)

## Code Structure
- **Header files**: Type definitions, enums, struct declarations, function prototypes
- **Implementation files**: Function implementations
- **Single responsibility**: Each file has a clear purpose (poker logic, server, client)

## Memory Management
- **Fixed buffers**: Uses fixed-size buffers (BUFFER_SIZE 4096) to avoid dynamic allocation
- **Thread safety**: Mutex locks (pthread_mutex_t) protect shared state
- **Stack allocation**: Prefers stack allocation over heap where possible

## Error Handling
- **Perror**: Uses `perror()` for system call errors
- **Fprintf to stderr**: Debug output goes to stderr
- **Return values**: Functions return -1 or NULL on error where applicable

## Threading Patterns
- **POSIX threads**: Uses pthread_create, pthread_join, pthread_detach
- **Mutex locking**: Always lock before accessing shared state, unlock after
- **Pattern**: Create mutex, lock, modify/read shared data, unlock

## Comments
- **Block comments**: Use `/* ... */` for multi-line explanations
- **Function comments**: Brief description above complex functions
- **Inline comments**: Minimal, only when logic is non-obvious

## Formatting
- **Indentation**: 4 spaces (no tabs)
- **Braces**: K&R style - opening brace on same line for functions and control structures
- **Line length**: Generally keep under 100 characters
- **Spacing**: Space after keywords (if, while, for), no space after function names

## Best Practices
- **Compiler flags**: Always compile with `-Wall -Wextra` to catch warnings
- **Type safety**: Use proper types (Rank, Suit enums) instead of raw ints where meaningful
- **Buffer safety**: Use `snprintf` instead of `sprintf`, `strncpy` instead of `strcpy`
- **Thread cleanup**: Join or detach threads to avoid resource leaks
