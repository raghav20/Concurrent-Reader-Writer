Concurrent-Reader-Writer
========================

A simple multiple readers/multiple writers application. Writers are threads writing into a message queue and readers read from the queue. Implement the application in C++ as a console Win32 process. The number of readers and writers is passed through the command line.   The writers read data from a (long enough) text file 10 bytes at a time and send it to the queue. Readers receive the data and store it into a different file. When the entire contents of the source file are transferred, writers stop. When the entire contents of the source file are written to the destination file, all readers stop and the application terminates. The source file and the destination file identical when the application terminates. 
