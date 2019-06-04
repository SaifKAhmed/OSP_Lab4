// Built on the Canvas code given by instructor
//
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/time.h>
#include <errno.h>
#include<pthread.h>
#include<unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_THREADS 4
#define MAX_DIRS 10000

pthread_t tid[MAX_THREADS];
int matches_found = 0; // Number of files or directories found with search term
char* top_level_dirs[MAX_DIRS]; // Array to store MAX_DIRS number of tlds
int tld_count = 0;  // Number of top level directories found


//takes a file/dir as argument, recurses,
// prints name if empty dir or not a dir (leaves)
void recur_file_search(char *file);
void check_match(char *file);
void first_level_pass(char *file);
void threaded_recursive(char * file);

//share search term globally (rather than passing recursively)
const char *search_term;


/********************************** M A I N ****************************************/
int main(int argc, char **argv) {
    if(argc != 3) {
        printf("Usage: my_file_search <search_term> <dir>\n");
        printf("Performs recursive file search for files/dirs matching\
               <search_term> starting at <dir>\n");
        exit(1);
    }
    
    //don't need to bother checking if term/directory are swapped, since we can't
    // know for sure which is which anyway
    search_term = argv[1];
    
    //start timer for recursive search
    struct timeval start, end;
    gettimeofday(&start, NULL);

    first_level_pass(argv[2]); // Process the top level diretory to find
                                // and find top level directories
    
    threaded_recursive(argv[2]);
    
    gettimeofday(&end, NULL);
    printf("Time: %ld\n", (end.tv_sec * 1000000 + end.tv_usec)
           - (start.tv_sec * 1000000 + start.tv_usec));
    printf("Matching files and directories found are: %d \n\n", matches_found);
    return 0;

}


// Check to see if the path contains the search term;
// print name if the search term is mpresent in the name.
/************************* check match for string ****************************/
void check_match(char *file){
    if(strstr(file, search_term) != NULL){
        matches_found++;
        printf("%s\n", file);
    }
}


//Do the first pass at the top level to find all subdirectores at the toplevel
//These directoies are stroed in an array to be processed by threads
//Also process any files at the top level
/**************** first level pass ********************************************/
void first_level_pass(char *file)  {
    DIR *start_directory = opendir(file);//check if directory is actually a file

    if(start_directory==NULL) return; // Arg 2 was NOT DIRECTORY
    struct dirent * cur_file; //For processing the directory entries
    
    // While we are able to read entried from the directory
    while ((cur_file = readdir(start_directory)) != NULL) {
        //create a string to place the full pathname of file found
        char *next_file_str = malloc(sizeof(char) * \
                                     strlen(cur_file->d_name) + \
                                     strlen(file) + 2);
        // Add the parent path and put a / at the end
        strncpy(next_file_str, file, strlen(file));
        strncpy(next_file_str + strlen(file), \
                "/", 1);
        //Add the current directory entry name
        strncpy(next_file_str + strlen(file) + 1, \
                cur_file->d_name, \
                strlen(cur_file->d_name) + 1);
        

        //if(cur_file-> d_type != DT_DIR) { // if the type is not directory
        if(start_directory == NULL) { // if the type is not directory
            check_match(next_file_str);  // Check for string match here.
        }  else if(start_directory != NULL &&
                   strcmp(cur_file->d_name,".")!=0 &&
                   strcmp(cur_file->d_name,"..")!=0 ) {// if it is a directory
                // Check if match and add to the list of paths array.
                check_match(next_file_str);
                top_level_dirs[tld_count] = malloc(sizeof(char)*255);
                strcpy(top_level_dirs[tld_count], next_file_str);
                tld_count++; //increment global count variable
            }
    }
    closedir(start_directory); // finally close the directory
}


// Create upto max threads without exceeding diretories to recurse
// Each top level directory is processed by a thread
// When all the threads are used up, wait from them to free and reuse them
// if there are more directories to process.
/****************  threaded recursive **************************************/
void threaded_recursive(char * file){
    pthread_t tid_array[MAX_THREADS]; //An array to store threads
    int count_down_dirs = 0; // Counter to keep track of directories done
    int thread_counter = 0; // Counter to keep track of threads used / completed
    

    do { // create threads
        for (thread_counter = 0; thread_counter < MAX_THREADS && \
             count_down_dirs < tld_count; \
             thread_counter++, count_down_dirs++) {
            pthread_create(&tid_array[thread_counter], NULL,\
                           recur_file_search, top_level_dirs[count_down_dirs]);
        }
        
        // Wait for threads to complete.
        for( int i = 0, j = thread_counter; i < j; i++, thread_counter-- ){
            if(pthread_join(tid_array[i], NULL)){ //Wait for threads to finish
                printf("Error in joining thread\n");
            } else {
                //printf("Completed thread %d\n", i);
            }
        }
    } while (count_down_dirs < tld_count); //WHile directores remain unprocessed
    
    return;
}



//Instructor provided with minor modifications
/********************** recursive file search Original ************************/
void recur_file_search(char *file)
{
    //check if directory is actually a file
    DIR *d = opendir(file);
    int skip = 0;
    
    //NULL means not a directory (or another, unlikely error)
    if(d == NULL)
    {
        //opendir SHOULD error with ENOTDIR, but if it did something else,
        // we have a problem (e.g., forgot to close open files, got
        // EMFILE or ENFILE)
        if(errno != ENOTDIR)
        {
            skip = 1;  // ***** Rather than exit, I'm skipping the entry.
            //perror("Something weird happened!");
            //fprintf(stderr, "While looking at: %s\n", file);
            //exit(1);
        }
        
        //nothing weird happened, check if the file contains the search term
        // and if so print the file to the screen (with full path)
        if (skip == 0){ // Nothing wierd happened, process file
            if(strstr(file, search_term) != NULL) {
                printf("%s\n", file);
                matches_found++; // Increment since it was a match
            }
        } else {
            skip = 0;
        }
        
        //no need to close d (we can't, it is NULL!)
        return;
    }
    
    //we have a directory, not a file, so check if its name
    // matches the search term
    if(strstr(file, search_term) != NULL) {
        printf("%s/\n", file);
        matches_found++;
    }
    
    //call recur_file_search for each file in d
    //readdir "discovers" all the files in d, one by one and we
    // recurse on those until we run out (readdir will return NULL)
    struct dirent *cur_file;
    while((cur_file = readdir(d)) != NULL) {
        //make sure we don't recurse on . or ..
        if(strcmp(cur_file->d_name, "..") != 0 &&\
           strcmp(cur_file->d_name, ".") != 0)
        {
            //we need to pass a full path to the recursive function,
            // so here we append the discovered filename (cur_file->d_name)
            // to the current path (file -- we know file is a directory at
            // this point)
            char *next_file_str = malloc(sizeof(char) * \
                                         strlen(cur_file->d_name) + \
                                         strlen(file) + 2);
            
            strncpy(next_file_str, file, strlen(file));
            strncpy(next_file_str + strlen(file), \
                    "/", 1);
            strncpy(next_file_str + strlen(file) + 1, \
                    cur_file->d_name, \
                    strlen(cur_file->d_name) + 1);
            
            //recurse on the file
            recur_file_search(next_file_str);
            
            //free the dynamically-allocated string
            free(next_file_str);
        }
    }
    
    //close the directory, or we will have too many files opened (bad times)
    closedir(d);
}




/*********************************  E N D *************************************/



