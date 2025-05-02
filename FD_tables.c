#define _GNU_SOURCE // Enable GNU extensions for DT_LNK, DT_DIR, etc.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h> 


void show_process_fd_table(pid_t pid) {
    ///_|> descry: Displays a table of file descriptors (FDs) for a specific process or all user processes
    ///_|> arg_i: pid, the process ID to analyze (-1 for th case of all processes owned by the current user), type pid_t
    ///_|> returning: This function does not return anything because of type void

    DIR *dir;                 // Directory too look at /proc OR /proc/[pid]/fd
    struct dirent *entry;    // This is used to read contents from the directory
    char path[32];           // Used to create directory paths
    int row_number = 0;      // For row numbering 

    if (pid != -1) {                                            // Specific PID case
        snprintf(path, sizeof(path), "/proc/%d/fd", pid);       // This builds the path to FD directory        
        dir = opendir(path);
        if (!dir) {
            perror("Cannot open FD directory");
            return;
        }
        // Print table header
        printf(">>> Process FD table for target PID: %d\n\n", pid);
        printf("         PID     FD \n");
        printf("        ============\n");

        // Iterate through FDs
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type != DT_LNK) continue;              // Skips the ones that are not FDs
            char fd_path[280];                                  // this is where the created FD path is stored.

            // Construct full path to FD
            snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%s", pid, entry->d_name);

            // Print row with row number
            printf("%14d   %-4s  \n", pid, entry->d_name);
        }
        closedir(dir);
        printf("        ============\n\n ");
    } else {                                                    // All user processes case{
        uid_t uid = getuid();                                   // Get current user's UID
        dir = opendir("/proc");
        if (!dir) {
            perror("Cannot open /proc");
            return;
        }
        // Print table header
        printf("         PID     FD \n");
        printf("        ============\n");

        // Iterate through /proc for all PIDs
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type != DT_DIR) continue;              // Skip non-directories
            pid_t proc_pid = atoi(entry->d_name);               // Convert directory name to PID
            if (proc_pid <= 0) continue;                        // Skip entries that are non-numeric

            // Check if process belongs to current user
            snprintf(path, sizeof(path), "/proc/%d", proc_pid);
            struct stat statbuf;
            if (stat(path, &statbuf) == -1) continue;           // Skip if inaccessible
            if (statbuf.st_uid != uid) continue;                // Skip if not owned by user

            // Open FD directory for this process
            snprintf(path, sizeof(path), "/proc/%d/fd", proc_pid);
            DIR *fd_dir = opendir(path);
            if (!fd_dir) continue; // Skip if inaccessible

            // Iterate through FDs
            struct dirent *fd_entry;
            while ((fd_entry = readdir(fd_dir)) != NULL) {
                if (fd_entry->d_type != DT_LNK) continue;         // Skip non-symbolic links

                char fd_path[280];                                // Used to store the full FD path

                snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%s", proc_pid, fd_entry->d_name);  //Constructs the FD path

                if (stat(fd_path, &statbuf) == -1) continue;      // Skip if FD stat fails

                // Print row with row number
                printf("%-8d %-7d %-5s\n", row_number++, proc_pid, fd_entry->d_name);
            }
            closedir(fd_dir);
        }
        closedir(dir);
        printf("        ============\n\n");
    }
}

void show_system_wide_fd_table(pid_t pid) {
    ///_|> descry: Shows a table of file descriptors (FDs) with filenames for a specific process or all user processes
    ///_|> arg_i: pid, the process ID to check (use -1 for all processes of the current user), type pid_t
    ///_|> returning: This function does not return anything, type void

    DIR *dir;             // This holds the directory are looking at
    struct dirent *entry; // This stores info about each file or folder we find
    char path[32];        // A small space to build directory paths
    int row_number = 0;   // Keeps track of row numbers for the table

    if (pid != -1) {
        // If we have a specific PID to check
        snprintf(path, sizeof(path), "/proc/%d/fd", pid); // Make the path to the FD folder
        dir = opendir(path);                              // Open that folder
        if (!dir) {
            perror("Cannot open FD directory");           // Show an error if it doesn't open
            return;                                       // Stop if we can't go on
        }
        
        // Print the table title and headers
        printf(">>> System wide FD table for target PID: %d\n\n", pid);
        printf("        PID     FD      Filename                 \n");
        printf("       ==========================================\n");

        // Look at each FD in the folder
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type != DT_LNK) continue;        // Skip if it's not a link (not an FD)
            char fd_path[280];                            // Big space for the full FD path
            char target[256];                             // Space to hold the filename the FD points to

            // Build the full path to this FD
            snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%s", pid, entry->d_name);

            // Find out what file this FD links to
            ssize_t len = readlink(fd_path, target, sizeof(target) - 1);
            if (len == -1) continue;                      // Skip if we can't read the link
            target[len] = '\0';                           // Add an end to the filename string

            // Show the PID, FD number, and filename in a row
            printf("%13d   %-7s %-23s \n", pid, entry->d_name, target);
        }
        closedir(dir);                                    // Done with this folder, so close it
        printf("       ==========================================\n\n");
    } else {
        // If we’re checking all processes for the current user
        uid_t uid = getuid();                             // Get the ID of the user running this
        dir = opendir("/proc");                           // Open the main /proc folder
        if (!dir) {
            perror("Cannot open /proc");                  // Show an error if it fails
            return;                                       // Stop if we can't go on
        }

        // Print the table headers
        printf("        PID     FD      Filename                 \n");
        printf("       ==========================================\n");

        // Go through each folder in /proc
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type != DT_DIR) continue;        // Skip if it's not a folder
            pid_t proc_pid = atoi(entry->d_name);         // Turn the folder name into a PID number
            if (proc_pid <= 0) continue;                  // Skip if it's not a real PID (like "self")

            // Check if this process belongs to our user
            snprintf(path, sizeof(path), "/proc/%d", proc_pid); // Make path to this process
            struct stat statbuf;                                // Space to hold info about the process
            if (stat(path, &statbuf) == -1) continue;           // Skip if we can't get info
            if (statbuf.st_uid != uid) continue;                // Skip if it's not our user's process

            // Open the FD folder for this process
            snprintf(path, sizeof(path), "/proc/%d/fd", proc_pid);
            DIR *fd_dir = opendir(path);                        // Open the FD folder
            if (!fd_dir) continue;                              // Skip if it won't open

            // Look at each FD in this process
            struct dirent *fd_entry;                            // Space for FD info
            while ((fd_entry = readdir(fd_dir)) != NULL) {
                if (fd_entry->d_type != DT_LNK) continue;       // Skip if it's not a link

                char fd_path[280];                              // Big space for the FD path
                char target[256];                               // Space for the filename

                // Build the path to this FD
                snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%s", proc_pid, fd_entry->d_name);
                ssize_t len = readlink(fd_path, target, sizeof(target) - 1);
                if (len == -1) continue;                        // Skip if we can't read the link
                target[len] = '\0';                             // End the filename string

                // Show the row with number, PID, FD, and filename
                printf("%-7d %-7d %-7s %-23s \n", row_number++, proc_pid, fd_entry->d_name, target);
            }
            closedir(fd_dir);                                   // Close the FD folder
        }
        closedir(dir);                                          // Close the /proc folder
        printf("       ==========================================\n\n");
    }
}

void show_vnodes_fd_table(pid_t pid) {
    ///_|> descry: Shows a table of file descriptors (FDs) and their inode numbers for a specific process or all user processes
    ///_|> arg_i: pid, the process ID to look at (use -1 for all processes of the current user), type pid_t
    ///_|> returning: This function does not return anything, type void

    DIR *dir;             // Holds the folder we are checking
    struct dirent *entry; // Keeps info about each file or folder we find
    char path[32];        // Small space to make folder paths
    int row_number = 0;   // Counts rows for the table

    if (pid != -1) {
        // If we are looking at just one PID
        snprintf(path, sizeof(path), "/proc/%d/fd", pid); // Build the path to the FD folder
        dir = opendir(path);                              // Open that folder
        if (!dir) {
            perror("Cannot open FD directory");           // Show an error if it fails
            return;                                       // Stop if we cannot go on
        }

        // Print the table title and headers
        printf(">>> Vnodes FD table for target PID: %d\n\n", pid);
        printf("             FD                       Inode\n");
        printf("       =================================================\n");

        // Go through each FD in the folder
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type != DT_LNK) continue;        // Skip if it is not a link (not an FD)
            char fd_path[280];                            // Big space for the full FD path
            struct stat statbuf;                          // Space to hold file info

            // Make the full path to this FD
            snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%s", pid, entry->d_name);

            // Get the inode number for this FD
            if (stat(fd_path, &statbuf) == -1) continue;  // Skip if we cannot get the info

            // Show the FD number and inode in a row
            printf("%14s %28lu\n", entry->d_name, statbuf.st_ino);
        }
        closedir(dir);                                   
        printf("       ===============================================\n\n");
    } else {
        // If we’re checking all processes for the user
        uid_t uid = getuid();                             // Get the ID of the user running this
        dir = opendir("/proc");                           // Open the main /proc folder
        if (!dir) {
            perror("Cannot open /proc");                  
            return;                                       // Stop if we cannot go on
        }

        // Print the table headers
        printf("             FD                      Inode\n");
        printf("       ===============================================\n");

        // Look at each folder in /proc
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type != DT_DIR) continue;        // Skip if it is not a folder
            pid_t proc_pid = atoi(entry->d_name);         // Turn folder name into a PID
            if (proc_pid <= 0) continue;                  // Skip if it is not a real PID

            // Check if this process is ours
            snprintf(path, sizeof(path), "/proc/%d", proc_pid); // Make path to this process
            struct stat statbuf;                                // Space for process info
            if (stat(path, &statbuf) == -1) continue;           // Skip if we can not check it
            if (statbuf.st_uid != uid) continue;                // Skip if it is not our user process

            // Open the FD folder for this process
            snprintf(path, sizeof(path), "/proc/%d/fd", proc_pid);
            DIR *fd_dir = opendir(path);                        // Open the FD folder
            if (!fd_dir) continue;                              // Skip if it does not open

            // Check each FD in this process
            struct dirent *fd_entry;                            // Space for FD info
            while ((fd_entry = readdir(fd_dir)) != NULL) {
                if (fd_entry->d_type != DT_LNK) continue;       // Skip if it is not a link

                char fd_path[280];                              // Big space for the FD path

                // Build the path to this FD
                snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%s", proc_pid, fd_entry->d_name);

                // Get the inode number
                if (stat(fd_path, &statbuf) == -1) continue;    // Skip if we can not get it

                // Show the row with number, FD, and inode
                printf("%-12d %-23s %lu\n", row_number++, fd_entry->d_name, statbuf.st_ino);
            }
            closedir(fd_dir);                                  
        }
        closedir(dir);                           
        printf("       ===============================================\n\n");
    }
}

void show_composite_table(pid_t pid, FILE *fp) {
    ///_|> descry: Shows a table with PID, FD, filename, and inode for a specific process or all user processes
    ///_|> arg_i: pid, the process ID to check (use -1 for all processes of the current user), type pid_t
    ///_|> arg_ii: fp, the file to write the table to (use NULL to print to screen), type FILE pointer
    ///_|> returning: This function does not return anything, type void

    DIR *dir;             // Holds the folder we are looking at
    struct dirent *entry; // Stores info about each file or folder
    char path[32];        // Small space to build folder paths
    int row_number = 0;   // Counts rows for the table

    int print_to_stdout = 0;  // Flag to decide if we print to screen
    if (fp == NULL) {
        print_to_stdout = 1;  // Set flag if no file is given
    }

    if (print_to_stdout == 1) {
        fp = stdout;          // Use the screen as output if no file
    }

    if (pid != -1) {
        // If we are checking just one PID
        snprintf(path, sizeof(path), "/proc/%d/fd", pid); // Make the path to the FD folder
        dir = opendir(path);                              // Open that folder
        if (!dir) {
            perror("Cannot open FD directory");           // Show an error if it fails
            return;                                       // Stop if we cannot go on
        }

        // Print the table title and headers to the file or screen
        fprintf(fp, ">>> Composite table for target PID: %d\n\n", pid);
        fprintf(fp, "        PID     FD      Filename                 Inode\n");
        fprintf(fp, "       =================================================\n");

        // Go through each FD in the folder
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type != DT_LNK) continue;        // Skip if it is not a link (not an FD)
            char fd_path[280];                            // Big space for the full FD path
            char target[256];                             // Space for the filename
            struct stat statbuf;                          // Space to hold file info

            // Build the full path to this FD
            snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%s", pid, entry->d_name);

            // Find the filename this FD points to
            ssize_t len = readlink(fd_path, target, sizeof(target) - 1);
            if (len == -1) continue;                      // Skip if we cannot read the link
            target[len] = '\0';                           // Add an end to the filename

            // Get the inode number for this FD
            if (stat(fd_path, &statbuf) == -1) continue;  // Skip if we cannot get the info

            // Show the PID, FD, filename, and inode in a row
            fprintf(fp, "%13d   %-5s   %-23s  %lu\n", pid, entry->d_name, target, statbuf.st_ino);
        }
        closedir(dir);                       
        fprintf(fp, "       ===============================================\n\n");
    } else {
        // If we are checking all processes for the user
        uid_t uid = getuid();                             // Get the ID of the user running this
        dir = opendir("/proc");                           // Open the main /proc folder
        if (!dir) {
            perror("Cannot open /proc");                  // Show an error if it fails
            return;                                       // Stop if we cannot go on
        }

        // Print the table headers to the file or screen
        fprintf(fp, "        PID     FD      Filename                Inode\n");
        fprintf(fp, "       ===============================================\n");

        // Look at each folder in /proc
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type != DT_DIR) continue;        // Skip if it is not a folder
            pid_t proc_pid = atoi(entry->d_name);         // Turn folder name into a PID
            if (proc_pid <= 0) continue;                  // Skip if it is not a real PID

            // Check if this process is ours
            snprintf(path, sizeof(path), "/proc/%d", proc_pid); // Make path to this process
            struct stat statbuf;                                // Space for process info
            if (stat(path, &statbuf) == -1) continue;           // Skip if we cannot check it
            if (statbuf.st_uid != uid) continue;                // Skip if it is not our user process

            // Open the FD folder for this process
            snprintf(path, sizeof(path), "/proc/%d/fd", proc_pid);
            DIR *fd_dir = opendir(path);                        // Open the FD folder
            if (!fd_dir) continue;                              // Skip if it does not open

            // Check each FD in this process
            struct dirent *fd_entry;                            // Space for FD info
            while ((fd_entry = readdir(fd_dir)) != NULL) {
                if (fd_entry->d_type != DT_LNK) continue;       // Skip if it is not a link

                char fd_path[280];                              // Big space for the FD path
                char target[256];                               // Space for the filename

                // Build the path to this FD
                snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%s", proc_pid, fd_entry->d_name);
                ssize_t len = readlink(fd_path, target, sizeof(target) - 1);
                if (len == -1) continue;                        // Skip if we cannot read the link
                target[len] = '\0';                             // End the filename string

                // Get the inode number
                if (stat(fd_path, &statbuf) == -1) continue;    // Skip if we cannot get it

                // Show the row with number, PID, FD, filename, and inode
                fprintf(fp, "%-7d %-7d %-7s %-23s %lu\n", row_number++, proc_pid, fd_entry->d_name, target, statbuf.st_ino);
            }
            closedir(fd_dir);                       
        }
        closedir(dir);                                  
        fprintf(fp, "       ===============================================\n\n");
    }
}

void show_summary_table() {
    ///_|> descry: Shows a summary table with each process ID and how many file descriptors it has
    ///_|> returning: This function does not return anything, type void

    // Print the table title
    printf("\n         Summary Table\n");
    printf("         =============\n");

    DIR *dir;             // Holds the /proc folder
    struct dirent *entry; // Stores info about each folder we find
    char path[4096];      // Big space to build folder paths

    dir = opendir("/proc");  
    if (!dir) {
        perror("Cannot open /proc");  // Show an error if it fails
        return;                       // Stop if we cannot go on
    }

    // Look at each folder in /proc
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_DIR) continue;  // Skip if it is not a folder
        pid_t proc_pid = atoi(entry->d_name);   // Turn folder name into a PID
        if (proc_pid <= 0) continue;            // Skip if it is not a real PID

        // Make the path to this process FD folder
        snprintf(path, sizeof(path), "/proc/%d/fd", proc_pid);
        DIR *fd_dir = opendir(path);            // Open the FD folder
        if (!fd_dir) continue;                  // Skip if it doesnot open

        int fd_count = 0;                       // Counter for FDs
        struct dirent *fd_entry;                // Space for FD info
        while ((fd_entry = readdir(fd_dir)) != NULL) {
            if (fd_entry->d_type != DT_LNK) continue;  // Skip if it is not a link (not an FD)
            fd_count++;                                // Add 1 for each FD we find
        }
        closedir(fd_dir);                              // Close the FD folder

        // Show the PID and FD count
        printf("%d (%d),  ", proc_pid, fd_count);
    }
    printf("\n\n");       
    closedir(dir);          
}

void show_threshold_exceeding(int threshold) {
    ///_|> descry: Shows a list of processes with more file descriptors than the given threshold
    ///_|> arg_i: threshold, the number of FDs a process must exceed to be shown, type int
    ///_|> returning: This function does not return anything, type void

    // Print the title with the threshold value
    printf("## Offering processes -- #FD threshold=%d\n", threshold);

    DIR *dir;             // Holds the /proc folder
    struct dirent *entry; // Stores info about each folder we find
    char path[4096];      // Big space to build folder paths

    dir = opendir("/proc");  // Open the main /proc folder
    if (!dir) {
        perror("Cannot open /proc");  // Show an error if it fails
        return;                       // Stop if we cannot go on
    }

    // Look at each folder in /proc
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_DIR) continue;  // Skip if it is not a folder
        pid_t proc_pid = atoi(entry->d_name);   // Turn folder name into a PID
        if (proc_pid <= 0) continue;            // Skip if it is not a real PID

        // Make the path to this process FD folder
        snprintf(path, sizeof(path), "/proc/%d/fd", proc_pid);
        DIR *fd_dir = opendir(path);            // Open the FD folder
        if (!fd_dir) continue;                  // Skip if it doesnot open

        int fd_count = 0;                       // Counter for FDs
        struct dirent *fd_entry;                // Space for FD info
        while ((fd_entry = readdir(fd_dir)) != NULL) {
            if (fd_entry->d_type != DT_LNK) continue;  // Skip if it is not a link (not an FD)
            fd_count++;                                // Add 1 for each FD we find
        }
        closedir(fd_dir);                              // Close the FD folder

        // Check if this process has more FDs than the threshold
        if (fd_count > threshold) {
            printf("%d (%d),  ", proc_pid, fd_count);  // Show PID and FD count if it is over the limit
        }
    }
    printf("\n\n");
    closedir(dir);
}

void create_output_txt(pid_t pid, char *filename) {
    ///_|> descry: Creates a text file and writes the composite table for a process or all user processes into it
    ///_|> arg_i: pid, the process ID to check (use -1 for all processes of the current user), type pid_t
    ///_|> arg_ii: filename, the name of the text file to create or update, type char pointer
    ///_|> returning: This function does not return anything, type void

    FILE *fp = fopen(filename, "w");  // Open the file for writing

    if (fp == NULL) {
        perror("Erroe: Failed to open text file\n");  // Show an error if the file does not open
        return;                                      // Stop if we cannot go on
    }

    show_composite_table(pid, fp);  // Write the composite table to the file
    fclose(fp);
    printf("\nThe TXT file has need created/updated\n\n");
}

void create_output_binary(pid_t pid, char *filename) {
    ///_|> descry: Creates a binary file and writes the composite table (PID, FD, filename, inode) for a process or all user processes
    ///_|> arg_i: pid, the process ID to check (use -1 for all processes of the current user), type pid_t
    ///_|> arg_ii: filename, the name of the binary file to create or update, type char pointer
    ///_|> returning: This function does not return anything, type void

    FILE *fp = fopen(filename, "wb");  // Open the file for writing in binary mode

    if (fp == NULL) {
        perror("Erroe: Failed to open binary file\n");  // Show an error if the file doesnt open
        return;                                        // Stop if we cannot go on
    }

    DIR *dir;             // Holds the folder we are checking
    struct dirent *entry; // Stores info about each folder or file
    char path[32];        // Small space for folder paths
    int row_number = 0;   // Counts rows for all-processes case

    // Write the table header to the file
    char header[] = "PID     FD      Filename                 Inode\n";
    int header_len = strlen(header);
    fwrite(&header_len, sizeof(int), 1, fp);  // Write header length first
    fwrite(header, sizeof(char), header_len, fp);  // Then write the header

    if (pid != -1) {
        // If we’re checking just one PID
        snprintf(path, sizeof(path), "/proc/%d/fd", pid);  // Build path to FD folder
        dir = opendir(path);                               // Open that folder
        if (!dir) {
            perror("Cannot open FD directory");            // Show error if it fails
            return;                                        // Stop if we cannot go on
        }

        // Write a separator line
        char separator[] = "       =================================================\n";
        int separator_len = strlen(separator);
        fwrite(&separator_len, sizeof(int), 1, fp);
        fwrite(separator, sizeof(char), separator_len, fp);

        // Go through each FD in the folder
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type != DT_LNK) continue;  // Skip if it is not a link (not an FD)
            char fd_path[280];                      // Big space for the FD path
            char target[256];                       // Space for the filename
            struct stat statbuf;                    // Space for file info

            snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%s", pid, entry->d_name);  // Build full FD path

            // Get the filename this FD points to
            ssize_t len = readlink(fd_path, target, sizeof(target) - 1);
            if (len == -1) continue;                // Skip if we cannot read it
            target[len] = '\0';                     // End the filename string

            if (stat(fd_path, &statbuf) == -1) continue;  // Skip if we cannot get file info

            // Write the PID, FD, filename, and inode to the file
            fwrite(&pid, sizeof(pid_t), 1, fp);
            int fd_len = strlen(entry->d_name);
            fwrite(&fd_len, sizeof(int), 1, fp);
            fwrite(entry->d_name, sizeof(char), fd_len, fp);
            int target_len = strlen(target);
            fwrite(&target_len, sizeof(int), 1, fp);
            fwrite(target, sizeof(char), target_len, fp);
            fwrite(&statbuf.st_ino, sizeof(statbuf.st_ino), 1, fp);
        }
        closedir(dir);

        // Write a footer line
        char footer[] = "       ===============================================\n";
        int footer_len = strlen(footer);
        fwrite(&footer_len, sizeof(int), 1, fp);
        fwrite(footer, sizeof(char), footer_len, fp);
    } else {
        // If we are checking all processes for the user
        uid_t uid = getuid();                             // Get the user ID
        dir = opendir("/proc");                           // Open the /proc folder
        if (!dir) {
            perror("Cannot open /proc");                  // Show error if it fails
            return;                                       // Stop if we cannot go on
        }

        // Write a separator line
        char separator[] = "       =================================================\n";
        int separator_len = strlen(separator);
        fwrite(&separator_len, sizeof(int), 1, fp);
        fwrite(separator, sizeof(char), separator_len, fp);

        // Look at each folder in /proc
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type != DT_DIR) continue;        // Skip if it is not a folder
            pid_t proc_pid = atoi(entry->d_name);         // Turn folder name into a PID
            if (proc_pid <= 0) continue;                  // Skip if it is not a real PID

            snprintf(path, sizeof(path), "/proc/%d", proc_pid);  // Build path to this process
            struct stat statbuf;                                 // Space for process info
            if (stat(path, &statbuf) == -1) continue;            // Skip if we cannot check it
            if (statbuf.st_uid != uid) continue;                 // Skip if it is not our user process

            snprintf(path, sizeof(path), "/proc/%d/fd", proc_pid);  // Path to FD folder
            DIR *fd_dir = opendir(path);                            // Open FD folder
            if (!fd_dir) continue;                                  // Skip if it does not open

            // Check each FD in this process
            struct dirent *fd_entry;
            while ((fd_entry = readdir(fd_dir)) != NULL) {
                if (fd_entry->d_type != DT_LNK) continue;  // Skip if it is not a link

                char fd_path[280];                         // Big space for FD path
                char target[256];                          // Space for filename

                snprintf(fd_path, sizeof(fd_path), "/proc/%d/fd/%s", proc_pid, fd_entry->d_name);
                ssize_t len = readlink(fd_path, target, sizeof(target) - 1);
                if (len == -1) continue;
                target[len] = '\0';

                if (stat(fd_path, &statbuf) == -1) continue;

                // Write row number, PID, FD, filename, and inode
                fwrite(&row_number, sizeof(int), 1, fp);
                row_number++;                              // Increase row count
                fwrite(&proc_pid, sizeof(pid_t), 1, fp);
                int fd_len = strlen(fd_entry->d_name);
                fwrite(&fd_len, sizeof(int), 1, fp);
                fwrite(fd_entry->d_name, sizeof(char), fd_len, fp);
                int target_len = strlen(target);
                fwrite(&target_len, sizeof(int), 1, fp);
                fwrite(target, sizeof(char), target_len, fp);
                fwrite(&statbuf.st_ino, sizeof(statbuf.st_ino), 1, fp);
            }
            closedir(fd_dir);
        }
        closedir(dir);

        // Write a footer line
        char footer[] = "       ===============================================\n";
        int footer_len = strlen(footer);
        fwrite(&footer_len, sizeof(int), 1, fp);
        fwrite(footer, sizeof(char), footer_len, fp);
    }

    printf("\nThe BINARY file has need created/updated\n\n");
}

int main(int argc, char *argv[]) {
    ///_|> descry: Main function that runs the program, checks command-line arguments, and calls the right functions
    ///_|> arg_i: argc, the number of arguments passed to the program, type int
    ///_|> arg_ii: argv, the array of argument strings passed to the program, type char pointer array
    ///_|> returning: Returns 0 if the program runs successfully, type int

    int threshold = -1;         // Default threshold value (means no threshold set)
    int show_per_process = 0;   // Flag for per-process table
    int show_system_wide = 0;   // Flag for system-wide table
    int show_vnodes = 0;        // Flag for vnodes table
    int show_composite = 0;     // Flag for composite table
    int show_summary = 0;       // Flag for summary table
    pid_t pid = -1;             // Default PID (means all processes)
    int output_txt = 0;         // Flag for text file output
    int output_binary = 0;      // Flag for binary file output

    // Check if there are too many arguments
    if (argc > 10) {
        fprintf(stderr, "Error: Too many arguments!!\n");
        exit(1);                // Stop the program with an error
    }

    // Look at each argument one by one
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--per-process") == 0) {
            show_per_process = 1;  // Turn on per-process table
        } else if (strcmp(argv[i], "--systemWide") == 0) {
            show_system_wide = 1;  // Turn on system-wide table
        } else if (strcmp(argv[i], "--Vnodes") == 0) {
            show_vnodes = 1;       // Turn on vnodes table
        } else if (strcmp(argv[i], "--composite") == 0) {
            show_composite = 1;    // Turn on composite table
        } else if (strcmp(argv[i], "--summary") == 0) {
            show_summary = 1;      // Turn on summary table
        } else if (strstr(argv[i], "--threshold=") == argv[i]) {
            char *threshold_val = argv[i] + 12;  // Grab the number after "--threshold="
            threshold = atoi(threshold_val);     // Turn it into an integer
            if (threshold <= 0) {
                fprintf(stderr, "Error: Threshold value is Invalid.\nIt must be a positive number!\n");
                exit(1);                         // Stop if it is not a good number
            }
        } else if (strcmp(argv[i], "--output_TXT") == 0) {
            output_txt = 1;                      // Turn on text file output
        } else if (strcmp(argv[i], "--output_binary") == 0) {
            output_binary = 1;                   // Turn on binary file output
        } else {
            // If it is not a flag, it might be a PID
            pid = atoi(argv[i]);
            if (pid <= 0) {
                fprintf(stderr, "Error: Invalid PID: %s\n", argv[i]);
                exit(1);                         // Stop if the PID is not valid
            }
        }
    }

    // If no options were picked, default to showing the composite table
    if ((show_per_process || show_system_wide || show_vnodes || show_composite || show_summary || threshold != -1 || output_txt || output_binary) == 0) {
        show_composite = 1;
    }

    // Run the functions based on the flags
    if (show_per_process == 1) {
        show_process_fd_table(pid);           // Show per-process table
    }
    if (show_system_wide == 1) {
        show_system_wide_fd_table(pid);       // Show system-wide table
    }
    if (show_vnodes == 1) {
        show_vnodes_fd_table(pid);            // Show vnodes table
    }
    if (show_composite == 1) {
        show_composite_table(pid, NULL);      // Show composite table on screen
    }
    if (show_summary == 1) {
        show_summary_table();                 // Show summary table
    }
    if (threshold != -1) {
        show_threshold_exceeding(threshold);  // Show processes over the threshold
    }
    if (output_txt == 1) {
        create_output_txt(pid, "compositeTable.txt");  // Make a text file
    }
    if (output_binary == 1) {
        create_output_binary(pid, "compositeTable.bin");  // Make a binary file
    }

    return 0;
}