#ifndef MPI_TOOLS_HPP
#define MPI_TOOLS_HPP

#include <vector>
#include <mpi.h>

template<typename T>
void mpi_send(const std::vector<T>& data, int dest, int tag, MPI_Comm comm) {
    if constexpr (std::is_same<T, int>::value) {
        MPI_Send(data.data(), data.size(), MPI_INT, dest, tag, comm);
    } else if constexpr (std::is_same<T, double>::value) {
        MPI_Send(data.data(), data.size(), MPI_DOUBLE, dest, tag, comm);
    } else {
        MPI_Send(data.data(), data.size() * sizeof(T), MPI_BYTE, dest, tag, comm);
    }
}

template<typename T>
void mpi_recv(std::vector<T>& data, int source, int tag, MPI_Comm comm) {
    if constexpr (std::is_same<T, int>::value) {
        MPI_Recv(data.data(), data.size(), MPI_INT, source, tag, comm, MPI_STATUS_IGNORE);
    } else if constexpr (std::is_same<T, double>::value) {
        MPI_Recv(data.data(), data.size(), MPI_DOUBLE, source, tag, comm, MPI_STATUS_IGNORE);
    } else {
        MPI_Recv(data.data(), data.size() * sizeof(T), MPI_BYTE, source, tag, comm, MPI_STATUS_IGNORE);
    }
}

template<typename T>
void mpi_sendrecv(std::vector<T>& sendbuff, int dest, int sendtag, std::vector<T>& recvbuff, int source, int recvtag, MPI_Comm comm) {
    if constexpr (std::is_same<T, int>::value) {
        MPI_Sendrecv(sendbuff.data(), sendbuff.size(), MPI_INT, dest, sendtag,
                     recvbuff.data(), recvbuff.size(), MPI_INT, source, recvtag,
                     comm, MPI_STATUS_IGNORE);
    } else if constexpr (std::is_same<T, double>::value) {
        MPI_Sendrecv(sendbuff.data(), sendbuff.size(), MPI_DOUBLE, dest, sendtag,
                     recvbuff.data(), recvbuff.size(), MPI_DOUBLE, source, recvtag,
                     comm, MPI_STATUS_IGNORE);
    } else {
        MPI_Sendrecv(sendbuff.data(), sendbuff.size() * sizeof(T), MPI_BYTE, dest, sendtag,
                     recvbuff.data(), recvbuff.size() * sizeof(T), MPI_BYTE, source, recvtag,
                     comm, MPI_STATUS_IGNORE);
    }
}

template <typename T>
void send_recv_data(std::vector<T>& send_data, int dest, std::vector<T>& recv_data, int source, int rank){
    // Odd ranks receive first, even ranks send first
    if (rank % 2 == 0) {
        mpi_send(send_data, dest, 0, MPI_COMM_WORLD);
        mpi_recv(recv_data, source, 0, MPI_COMM_WORLD);
    } else {
        mpi_recv(recv_data, source, 0, MPI_COMM_WORLD);
        mpi_send(send_data, dest, 0, MPI_COMM_WORLD);
    }
}

template<typename T>
void mpi_isend(const std::vector<T>& data, int dest, int tag, MPI_Comm comm, MPI_Request& request) {
    if constexpr (std::is_same<T, int>::value) {
        MPI_Isend(data.data(), data.size(), MPI_INT, dest, tag, comm, &request);
    } else if constexpr (std::is_same<T, double>::value) {
        MPI_Isend(data.data(), data.size(), MPI_DOUBLE, dest, tag, comm, &request);
    } else {
        MPI_Isend(data.data(), data.size() * sizeof(T), MPI_BYTE, dest, tag, comm, &request);
    }
}

template<typename T>
void mpi_irecv(std::vector<T>& data, int source, int tag, MPI_Comm comm, MPI_Request& request) {
    if constexpr (std::is_same<T, int>::value) {
        MPI_Irecv(data.data(), data.size(), MPI_INT, source, tag, comm, &request);
    } else if constexpr (std::is_same<T, double>::value) {
        MPI_Irecv(data.data(), data.size(), MPI_DOUBLE, source, tag, comm, &request);
    } else {
        MPI_Irecv(data.data(), data.size() * sizeof(T), MPI_BYTE, source, tag, comm, &request);
    }
}

template <typename T>
void isend_irecv_data(std::vector<T>& send_data, int dest, std::vector<T>& recv_data, int source, MPI_Comm comm) {
    MPI_Request requests[2];
    
    // Initiate non-blocking send and receive
    mpi_isend(send_data, dest, 0, comm, requests[0]);
    mpi_irecv(recv_data, source, 0, comm, requests[1]);

    // Wait for both send and receive to complete
    MPI_Waitall(2, requests, MPI_STATUSES_IGNORE);
}

#endif
