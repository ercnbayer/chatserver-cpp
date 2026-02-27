#pragma  once

#include <string>
#include <string_view>

namespace ChatProtocol {

    enum class ValidationError {
        NONE,
        TOO_SHORT,
        NO_SLASH,
        CONTROL_CHARS,
        MALFORMED_UTF8,
        BUFFER_OVERFLOW
    };

    class Validator {
    private:
        /**
         * Checks if a string conforms to valid UTF-8 structural rules.
         * This prevents "overlong encoding" and "invalid byte sequences" 
         * which are common in binary-level attacks.
         */
        static bool is_valid_utf8(const std::string_view& str) {
            int expected_bytes = 0;
            for (unsigned char c : str) {
                if (expected_bytes == 0) {
                    if (c <= 0x7F) expected_bytes = 0; // Standard 1-byte ASCII
                    else if ((c & 0xE0) == 0xC0) expected_bytes = 1; // 2-byte sequence (e.g., Turkish chars)
                    else if ((c & 0xF0) == 0xE0) expected_bytes = 2; // 3-byte sequence
                    else if ((c & 0xF8) == 0xF0) expected_bytes = 3; // 4-byte sequence
                    else return false; // Invalid start byte
                } else {
                    // Continuation bytes must start with '10' binary pattern (0x80 to 0xBF)
                    if ((c & 0xC0) != 0x80) return false;
                    expected_bytes--;
                }
            }
            return expected_bytes == 0; // Ensure no partial sequences remain
        }

    public:
        /**
         * Orchestrates all validation checks for incoming raw messages.
         */
        static ValidationError validate_raw(const std::string_view& msg, size_t max_limit = 4096) {
            // Check basic length requirements
            if (msg.empty() || msg.length() < 2) return ValidationError::TOO_SHORT;
            
            // Check memory safety limit
            if (msg.length() > max_limit) return ValidationError::BUFFER_OVERFLOW;

            // Enforce command-style protocol (must start with '/')
            if (msg[0] != '/') return ValidationError::NO_SLASH;

            // Detect illegal ASCII control characters (0-31) except Tab
            for (unsigned char c : msg) {
                if (c < 32 && c != '\t') {
                    return ValidationError::CONTROL_CHARS;
                }
            }

            // Perform deep structural check for UTF-8 integrity
            if (!is_valid_utf8(msg)) {
                return ValidationError::MALFORMED_UTF8;
            }

            return ValidationError::NONE;
        }

        /**
         * Translates internal error codes to user-friendly/log-friendly strings.
         */
        static std::string error_to_string(ValidationError err) {
            switch (err) {
                case ValidationError::TOO_SHORT:      return "Message is too short";
                case ValidationError::NO_SLASH:       return "Commands must start with '/'";
                case ValidationError::CONTROL_CHARS:  return "Binary control characters detected";
                case ValidationError::MALFORMED_UTF8: return "Invalid UTF-8 sequence detected";
                case ValidationError::BUFFER_OVERFLOW: return "Message exceeds buffer limit";
                default:                             return "Unknown protocol error";
            }
        }
    };
}

