/**
 * PRL Project 1 - Odd-Even Transposition Sort
 * @author: Michal Blazek (xblaze34)
 * @date: 2025
 * @file: oets.cpp
 */

#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>

#define ROOT 0 // Rank of the root process


/**
 * Create a vector of 8bit numbers from the file numbers
 * @return A vector of 8bit numbers
 */
std::vector<unsigned char> load_data();


/**
 * Send or recieve the number from the neighbour process
 * @param my_number The number assigned to the current process
 * @param recieved_number The number recieved from the neighbour process
 * @param rank The rank of the current process
 * @param size The total number of processes
 * @param sender_flag The flag to determine if the current process is the sender or the reciever
 */
void send_recieve(unsigned char* my_number, unsigned char* recieved_number, int rank, int size, int sender_flag);


/**
 * Swap the numbers between the current process and the neighbour process
 * @param my_number The number assigned to the current process
 * @param recieved_number The number recieved from the neighbour process
 * @param rank The rank of the current process
 * @param size The total number of processes
 * @param sender_flag The flag to determine if the current process is the sender or the reciever
 */
void swap_numbers(unsigned char* my_number, unsigned char* recieved_number, int rank, int size, int sender_flag);


int main(int argc, char *argv[]) {
    int rank;                           // Rank of the current process
    int size;                           // Total number of processes
    std::vector<unsigned char> numbers; // Numbers to sort
    unsigned char my_number;            // The number currently assigned to the process
    unsigned char recieved_number;      // The number recieved from neighbour process

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Process with rank 0 loads the data
    if (rank == ROOT) {
        numbers = load_data();

        // Print the numbers
        for (auto i: numbers) {
            std::cout << +i << ' ';
        }
        std::cout << std::endl;
    }

    // Scatter the data among all the processes
    // -> Each process recieves one number
    MPI_Scatter(numbers.data(), 1, MPI_UNSIGNED_CHAR, &my_number, 1, MPI_UNSIGNED_CHAR, ROOT, MPI_COMM_WORLD);

    // Sort the numbers
    for (int i = 0; i < size; i++) {
        // Rank % 2 == (i%2) -> send my number to the right neighbour
        // Rank % 2 != (i%2) -> recieve the number from the left neighbour
        send_recieve(&my_number, &recieved_number, rank, size, i % 2);

        // Swap the numbers between the current process and the neighbour process
        swap_numbers(&my_number, &recieved_number, rank, size, i % 2);
    }

    // Gather the sorted numbers
    MPI_Gather(&my_number, 1, MPI_UNSIGNED_CHAR, numbers.data(), 1, MPI_UNSIGNED_CHAR, ROOT, MPI_COMM_WORLD);

    // Print the sorted numbers
    if (rank == ROOT) {
        for (auto i: numbers) {
            std::cout << +i << std::endl;
        }
    }

    MPI_Finalize();
    return 0;
}


void swap_numbers(unsigned char* my_number, unsigned char* recieved_number, int rank, int size, int sender_flag) {
    // Recieved number from neigbour in this iteration
    // -> Send the correct number back
    if (rank % 2 != sender_flag) {
        int neighbour_rank = rank - 1; // Rank of the left neighbour
        unsigned char temp; // Temporary variable to store the number

        // The first process has no left neighbour
        if (neighbour_rank < 0) {
            return;
        }

        // Swap numbers if the recieved number is greater than the current number
        if (*recieved_number > *my_number) {
            temp = *my_number;
            *my_number = *recieved_number;
        }
        else{
            // Otherwise just send the same number back
            temp = *recieved_number;
        }
        // Send the number to the neighbour
        MPI_Send(&temp, 1, MPI_UNSIGNED_CHAR, neighbour_rank, 0, MPI_COMM_WORLD);
    }
    else {
        // Sent number to neighbour in this iteration
        // -> Recieve the (potentially) swapped number
        int neighbour_rank = rank + 1; // Rank of the right neighbour

        // The last process has no right neighbour
        if (neighbour_rank >= size) {
            return;
        }

        // Recieve the number from the neighbour
        MPI_Recv(my_number, 1, MPI_UNSIGNED_CHAR, neighbour_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}


void send_recieve(unsigned char* my_number, unsigned char* recieved_number, int rank, int size, int sender_flag) {
    if (rank % 2 == sender_flag) {
        int recv_rank = rank + 1;

        // If the rank is greater or equal than the size, then the process has no right neighbour
        if (recv_rank >= size) {
            return;
        }

        // Send my number to the neighbour
        MPI_Send(my_number, 1, MPI_UNSIGNED_CHAR, rank+1, 0, MPI_COMM_WORLD);
    }

    else {
        int sender_rank = rank - 1;

        // If the rank is less than 0, then the process has no left neighbour
        if (sender_rank < 0) {
            return;
        }

        // Recieve the number from the neighbour
        MPI_Recv(recieved_number, 1, MPI_UNSIGNED_CHAR, sender_rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}


std::vector<unsigned char> load_data() {
    // Expecting a file called numbers in the current directory
    const char* filename = "numbers";

    // Open the file in binary mode for reading
    std::ifstream file(filename, std::ios::binary);

    // Check if the file was opened successfully
    if (!file) {
        std::cerr << "Error: Failed to open input file." << std::endl;
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Determine the size of the file
    file.seekg(0, std::ios::end);
    std::size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg); // Reset the file pointer

    // Create a vector to store the data
    std::vector<unsigned char> data(fileSize);

    // Read the file into the vector
    file.read((char*)&data[0], fileSize);

    // Close the file
    file.close();

    return data;
}