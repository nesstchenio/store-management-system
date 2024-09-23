# Store Management System

## Language

The project is entirely written in C.

## Architecture

The project is based on a **client/server** architecture with static worker management. The system's client-server architecture is static, meaning that each client is assigned a worker upon connection. When a client disconnects, the associated worker is placed on standby. The server is designed to handle up to **10 concurrent workers**. If an 11th client or more attempts to connect, it will be placed in a waiting state until a worker becomes available.


### Sockets

Communication between the client and server is managed using **sockets**. Each client connects to the server via a specified port, ensuring a stable exchange of data between the two processes.

### Workflow

1. Start the server on a given port.
2. Clients connect to the server on the same port and interact through predefined menus based on their role (cashier, stock manager, or manager).

## Requirements

- **Make** to compile the project.
- A terminal to run both the server and client.

## Compilation and Execution

1. Compile the project using the command:
   **make**

2. Start the server:
    ./serveur {port_number}

3. In another terminal, start the client:
    ./client localhost {port_number}

Make sure to use the same port number as for the server.

## Main Menu
Once the client is connected, the following main menu will be displayed:

- Cashier
- Stock Manager
- Manager
- Logout

## Client Roles
1. Cashier
    - Add Customer: Allows the cashier to add a new customer to the store's database.
    - Register Sale: Updates the stock levels and the loyalty points of the purchasing customer. If the customer is not in the database, the system will prompt to add the customer first.

2. Stock Manager
    - Update Stock: Enables the stock manager to update the inventory of the store.
    - Add New Item: Adds a new product to the store's catalog.

3. Manager
    - Add Employee: Allows the manager to add a new employee to the employee database.
