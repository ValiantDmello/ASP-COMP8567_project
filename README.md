# Client-Server Project: File Retrieval System in C

This repository contains the source code for a client-server project developed for the COMP-8567 course in Winter 2023. The project focuses on creating a file retrieval system where clients can request files or sets of files from the server. The system allows multiple clients to connect to the server and retrieve files based on specific commands.

## Project Overview

In this project, the system is designed to operate with the following key components:

* Server: The server process is responsible for handling client requests. It can manage multiple client connections concurrently. Upon receiving a connection request from a client, the server forks a child process that services the client request exclusively. The server returns to listening for requests from other clients. The server's main function is to process client requests according to the specified commands.

* Mirror: An identical copy of the server, often with some additional features, can be used to handle client connections in an alternating manner. The first four client connections are handled by the server, the next four by the mirror, and the rest are alternated between the server and the mirror.

* Client: The client process runs an infinite loop, waiting for user input. Clients can enter specific commands to request files from the server or mirror. The client verifies the syntax of the commands and sends them to the server for processing. The client is responsible for printing the received information or error messages.

### Client Commands

The following commands can be used by clients to interact with the server:

1. findfile filename: Requests information about a specific file. If found, the server returns the filename, size, and date created. If the file exists in multiple locations, information from the first successful search is provided.

2. sgetfiles size1 size2 <-u>: Requests a compressed file containing all files with sizes within the specified range. The -u flag indicates unzipping the file after retrieval.

3. dgetfiles date1 date2 <-u>: Requests files created within a specific date range. The -u flag indicates unzipping the file after retrieval.

4. getfiles file1 file2 file3 file4 file5 file6 <-u>: Requests a compressed file containing specified files. If none are present, the server sends "No file found." The -u flag indicates unzipping the file after retrieval.

4. gettargz <extension list> <-u>: Requests files with specific file extensions. The -u flag indicates unzipping the file after retrieval.

6. quit: Terminates the client process.

## Project Structure

* server.c: Contains the source code for the server component.
* mirror.c: Contains the source code for the mirror component.
* client.c: Contains the source code for the client component.

## Running the Project

To run the project, compile and execute the server, mirror, and client components on separate machines or terminals. Ensure that the server and mirror handle client connections as per the specified rules.
