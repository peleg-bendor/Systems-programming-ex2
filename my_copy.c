/*
 * my_copy.c - File copy program using system calls
 * 
 * Version 3: Added destination file existence check and user confirmation
 * 
 * Usage: ./my_copy <source_file> <destination_file>
 * 
 * Uses ONLY system calls - no standard C library functions like printf/fopen!
 */

#include <unistd.h>    // for read(), write(), close(), access()
#include <fcntl.h>     // for open(), O_RDONLY, O_WRONLY, O_CREAT, O_TRUNC

#define BUFFER_SIZE 4096  // 4 KB buffer (optimal for most systems)

/*
 * Helper function: Calculate string length
 * 
 * We need this because we can't use strlen() from the standard library!
 * 
 * Returns the number of characters before the null terminator '\0'
 */
int string_length(const char *str) {
    int len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

int main(int argc, char *argv[]) {
    /*
     * Step 1: Check command-line arguments
     * 
     * argc = argument count (includes program name)
     * argv = argument vector (array of strings)
     * 
     * Expected:
     * argv[0] = "./my_copy" (program name)
     * argv[1] = source file name
     * argv[2] = destination file name
     * 
     * So argc should be 3
     */
    if (argc != 3) {
        char usage[] = "Usage: ./my_copy <source_file> <destination_file>\n";
        write(STDERR_FILENO, usage, sizeof(usage) - 1);
        return 1;
    }
    
    /*
     * Store the file names in readable variables
     * argv[1] = source file
     * argv[2] = destination file
     */
    char *source_file = argv[1];
    char *dest_file = argv[2];
    
    /*
     * Step 2: Check if destination file already exists
     * 
     * access() checks if a file exists and if we have certain permissions
     * F_OK = just check if file exists
     * 
     * Returns 0 if file exists, -1 if it doesn't (or other error)
     */
    if (access(dest_file, F_OK) == 0) {
        /*
         * Destination file exists!
         * We need to ask the user if they want to overwrite it.
         */
        char prompt1[] = "Destination file '";
        char prompt2[] = "' already exists. Copying will overwrite it. Continue? (y/n): ";
        
        write(STDOUT_FILENO, prompt1, sizeof(prompt1) - 1);
        write(STDOUT_FILENO, dest_file, string_length(dest_file));
        write(STDOUT_FILENO, prompt2, sizeof(prompt2) - 1);
        
        /*
         * Read user's response
         * We'll keep asking until we get 'y' or 'n'
         */
        char response;
        char newline;
        int valid_input = 0;
        
        while (!valid_input) {
            /*
             * Read one character from stdin (file descriptor 0)
             * The user will type 'y' or 'n' followed by Enter
             */
            ssize_t bytes_read = read(STDIN_FILENO, &response, 1);
            
            if (bytes_read == -1) {
                char error[] = "Error: Failed to read user input\n";
                write(STDERR_FILENO, error, sizeof(error) - 1);
                return 1;
            }
            
            /*
             * Also read the newline character that follows
             * (when user presses Enter)
             */
            read(STDIN_FILENO, &newline, 1);
            
            /*
             * Check if response is valid ('y' or 'n')
             */
            if (response == 'y' || response == 'Y') {
                // User confirmed - we'll continue with the copy
                valid_input = 1;
                char msg[] = "Proceeding with copy...\n";
                write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            }
            else if (response == 'n' || response == 'N') {
                // User cancelled - exit the program
                char msg[] = "Copy cancelled by user.\n";
                write(STDOUT_FILENO, msg, sizeof(msg) - 1);
                return 0;  // Exit successfully (user's choice)
            }
            else {
                // Invalid input - ask again
                char msg[] = "Invalid input. Please enter 'y' or 'n': ";
                write(STDOUT_FILENO, msg, sizeof(msg) - 1);
            }
        }
    }
    
    /*
     * Step 3: Open the source file for reading
     * 
     * O_RDONLY = Open for reading only
     * 
     * If the file doesn't exist or we don't have permission,
     * open() will return -1
     */
    int source_fd = open(source_file, O_RDONLY);
    
    if (source_fd == -1) {
        // Error opening source file
        char error1[] = "Error: Cannot open source file '";
        char error2[] = "'\n";
        write(STDERR_FILENO, error1, sizeof(error1) - 1);
        write(STDERR_FILENO, source_file, string_length(source_file));
        write(STDERR_FILENO, error2, sizeof(error2) - 1);
        return 1;
    }
    
    /*
     * Step 4: Create/open the destination file for writing
     * 
     * Flags:
     * - O_WRONLY: Open for writing only
     * - O_CREAT:  Create file if it doesn't exist
     * - O_TRUNC:  Truncate (empty) the file if it exists
     * 
     * Mode 0644: rw-r--r-- (owner can read/write, others can read)
     */
    int dest_fd = open(dest_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    
    if (dest_fd == -1) {
        // Error opening destination file
        char error1[] = "Error: Cannot create destination file '";
        char error2[] = "'\n";
        write(STDERR_FILENO, error1, sizeof(error1) - 1);
        write(STDERR_FILENO, dest_file, string_length(dest_file));
        write(STDERR_FILENO, error2, sizeof(error2) - 1);
        close(source_fd);  // Don't forget to close the source file!
        return 1;
    }
    
    /*
     * Step 5: Copy the file contents
     * 
     * We'll use a buffer to read chunks of data from source
     * and write them to destination.
     * 
     * This is more efficient than reading/writing one byte at a time!
     */
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    /*
     * Read loop:
     * - read() returns the number of bytes actually read
     * - Returns 0 when we reach end of file (EOF)
     * - Returns -1 on error
     */
    while ((bytes_read = read(source_fd, buffer, BUFFER_SIZE)) > 0) {
        /*
         * Write what we just read to the destination file
         * 
         * Important: write exactly bytes_read bytes,
         * not BUFFER_SIZE (the last chunk might be smaller!)
         */
        ssize_t bytes_written = write(dest_fd, buffer, bytes_read);
        
        if (bytes_written == -1) {
            char error[] = "Error: Failed to write to destination file\n";
            write(STDERR_FILENO, error, sizeof(error) - 1);
            close(source_fd);
            close(dest_fd);
            return 1;
        }
        
        /*
         * Sanity check: make sure we wrote all the bytes we read
         * (This should always be true for regular files)
         */
        if (bytes_written != bytes_read) {
            char error[] = "Error: Incomplete write\n";
            write(STDERR_FILENO, error, sizeof(error) - 1);
            close(source_fd);
            close(dest_fd);
            return 1;
        }
    }
    
    /*
     * Check if read() failed (vs. just reaching EOF)
     */
    if (bytes_read == -1) {
        char error[] = "Error: Failed to read from source file\n";
        write(STDERR_FILENO, error, sizeof(error) - 1);
        close(source_fd);
        close(dest_fd);
        return 1;
    }
    
    /*
     * Step 6: Close both files
     */
    if (close(source_fd) == -1) {
        char error[] = "Error: Failed to close source file\n";
        write(STDERR_FILENO, error, sizeof(error) - 1);
        close(dest_fd);
        return 1;
    }
    
    if (close(dest_fd) == -1) {
        char error[] = "Error: Failed to close destination file\n";
        write(STDERR_FILENO, error, sizeof(error) - 1);
        return 1;
    }
    
    /*
     * Step 7: Print success message
     */
    char success1[] = "Success! Copied '";
    char success2[] = "' to '";
    char success3[] = "'\n";
    write(STDOUT_FILENO, success1, sizeof(success1) - 1);
    write(STDOUT_FILENO, source_file, string_length(source_file));
    write(STDOUT_FILENO, success2, sizeof(success2) - 1);
    write(STDOUT_FILENO, dest_file, string_length(dest_file));
    write(STDOUT_FILENO, success3, sizeof(success3) - 1);
    
    return 0;  // Success!
}