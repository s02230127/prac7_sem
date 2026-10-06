# AI track

## Runtime agent

- Local model: `qwen2.5:3b`
- Runtime: Ollama (`http://127.0.0.1:11434`)
- AI task: daytime discussion and voting for one selected player
- Input context: role, personality, living players, known mafia allies and public round history
- Output: JSON `{action, message, target, reasoning}`
- API calls: at most one request per round because the game has one `--ai-player`
- Fallback: if Ollama is unavailable or the answer is invalid, the player uses the normal `discuss()` and `vote()` logic
