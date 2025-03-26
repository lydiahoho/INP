# Chat Room

## Description
In this homework, you are asked to design a chat server. Your program should be a **single process** and use [select](https://man7.org/linux/man-pages/man2/select.2.html) or [poll](https://man7.org/linux/man-pages/man2/poll.2.html) to handle **multiple connections** and receive user commands from the client socket. After receiving the command message, the server should send the corresponding message back.

:::danger
The TAs use the diff tool to compare your output directly against our prepared sample test data. When comparing against the sample test data, continuous spaces and tabs in the output are consolidated into a single space character.
:::

Your server should show a welcome message once a client is connected.
```
*********************************
** Welcome to the Chat server. **
*********************************
```

## Basic commands
The server accepts commands sent from the users, processes the commands, and responds to the users. The details of valid commands are introduced as follows.

:::warning
If a client sends an incomplete command, e.g., missing required parameters, the server should show the client the correct command format (usage). 
::: 
    
- register
    > register \<username> \<password>
    1. If there is any missing or redundant parameter
        - **`Usage: register <username> <password>`**
    2. If `<username>` already exists
        - **`Username is already used.`**
    3. If a client successfully registers an account
        - **`Register successfully.`**
    
    <br>
    
    :::info
    :bulb:The username and password only contain one word. Their respective length is less than 20 characters.
    :::

- login
    > login \<username> \<password>
    1. If there is any missing or redundant parameter
        - **`Usage: login <username> <password>`**
    2. If the client has already logged in with an account
        - **`Please logout first.`**
    3. If the username does not exist
        - **`Login failed.`**
    4. If the password is wrong 
        - **`Login failed.`**
    5. If an account is already logged in by another client
        - **`Login failed.`**
    6. If a client successfully logs in
        - **`Welcome, <username>.`**
        <br>
    :::info
    :bulb: If client A has already successfully logged in as user A, other clients (including client A) cannot log in as user A until client A logs out.
    :::
- logout
    > logout
    1. If there is any missing or redundant parameter
        - **`Usage: logout`**
    2. If the client has not logged in
        - **`Please login first.`**
    3. If the client successfully logs out
        - **`Bye, <username>.`**


- exit
    > exit
    1. If there is any missing or redundant parameter
        - **`Usage: exit`**
    2. If the client has not logged out, run the `logout` command first and then the `exit` command implicitly. The terminal would show:
        - **`Bye, <username>.`**
    3. If the exit command works successfully, the connection will be closed.
    
    <br>
    

- whoami
    > whoami
    1. If there is any missing or redundant parameter:
        - **`Usage: whoami`**
    2. If the client has not logged in:
        - **`Please login first.`**
    3. Show the username:
        - **`<username>`**
    
- set-status
    > set-status <status>
    <status>: `online`, `offline`, `busy`
    1. If there is any missing or redundant parameter:
        - **`Usage: set-status <status>`**
    2. If the client has not logged in:
        - **`Please login first.`**
    3. The status should be `online`, `offline`, or `busy`. If there is any other status:
        - **`set-status failed`**
    4. The default status for all users is `offline`:
    5. If setting successfully:
        - **`<username> <status>`**
    
    <br>
    
    :::info
    :bulb: The status of a user should be set to `online` after login. The status of a user should be set to `offline` after logging out or exiting. You do not need to show any set-status-related messages when invoking `login`, `logout`, and `exit` commands. 
    :::
    
- list-user
    > list-user
    1. If there is any missing or redundant parameter:
        - **`Usage: list-user`**
    2. If the client has not logged in:
        - **`Please login first.`**
    3. List all users and the corresponding status in the server and sort by user names alphabetically.
        - **`<username 1> <status>`**
        - **`<username 2> <status>`**
        - **`...`**
    

- Enter a chat room
    > enter-chat-room \<number>
    1. If there is any missing or redundant parameter:
        - **`Usage: enter-chat-room <number>`**
    2. If `<number>` is not a valid room number(not between 1 to 100)
        - **`Number <number> is not valid.`**
    3. If the client has not logged in:
        - **`Please login first.`**
    4. If the chat room does not exist, create a new room and enter this room:
        - **`Welcome to the public chat room.`**
        - **`Room number: <number>`**
        - **`Owner: <creator>`**
    5. If one client successfully enters the chat room:
        - Show the message to all clients in the chat room.
            - **`<username> had enter the chat room.`** 
        - Show the messages below to the new client.
            - **`Welcome to the public chat room.`**
            - **`Room number: <number>`**
            - **`Owner: <creator>`**
            - **`<chat_history>`**
        - Format of **<chat_history>**: 
            - Chat history is composed of several **Record**s.
                - `Record 1` + `Record 2` + ... + `Record n`
            - The format of a **Record** is
                - **`[<username>]: <message>\n`**
            - Only show the latest 10 Records. 
    
    <br>
    
    :::info
    :bulb: Every **\<message>** in **\<chat_history>** should also be filtered (see [here](https://md.zoolab.org/OFndeXQNQjGCj6USJlqStQ?both#Content-Filtering)). If there is a pin message, add a mark after `Record n` in the format **`Pin -> [<username>]: <message>\n`**.(see [here](https://md.zoolab.org/OFndeXQNQjGCj6USJlqStQ?both#Requirement-command-in-Chat-Room))
    :::
    
- list chat room
    > list-chat-room
    1. If there is any missing or redundant parameter:
        - **`Usage: list-chat-room`**
    2. If the client has not logged in:
        - **`Please login first.`**
    3. List all existing chat rooms and the corresponding owners in the server and sort by the room number.
        - **`<owner username> <room number>`**
        - **`...`**
    
- close chat room
    > close-chat-room <number>
    1. If there is any missing or redundant parameter:
        - **`Usage: close-chat-room <number>`**
    2. If the client has not logged in:
        - **`Please login first.`**
    3. If the user is not the owner:
        - **`Only the owner can close this chat room.`**
    4. If the chat room does not exist:
        - **`Chat room <number> does not exist.`**
    5. After closing successfully:
        - **`Chat room <number> was closed.`**
    
    <br>
    
    :::info
    :bulb: All users in a chat room must be kicked out when the room no longer exists. Once a chat room is closed, the server should show a message in the format **`Chat room <number> was closed.`** to all users in the chat room.
    :::

 - unknown command
    - Show the error message if the input command is not specified:
        - **`Error: Unknown command`**
        
<!-- - block
    > block <option> [username]
    1. if there is any missing parameter
        - **`Usage: block <option> [username]`**
    2. if the client has not logged in
        - **`Please login first.`**
    3. The option should be `-h`, `-l`, `-u`, or `-b`. If there is anyother status, show the content of `-h` Only `-u`, `-b` need to use [username].
    4. If the command is `block -h`
        ```
        Usuage: block <option> [username]
            options:
                -h:    print help message
                -l:    list blocked users
                -u:    unblock the given user
                -b:    block the given user
        ```
    5. If the command is `block -l`, list the blocked users of the current user. The Output is sorted by user name alphabetically. 
        - **`<username>`**
        - **`...`**
    6. If the option is `-b`, block the given user. If no user given, show the content of `-h`.
    7. If the option is `-u`, remove the given user from the block list of the current user. If no user given, show the content of `-h`.

    <br>
    
    :::info
    :bulb: If UserA is blocked by UserB, UserB will not see any information of UserA, which includes `list-user`, `list-chat-room`, message of UserA, `chat_history`, and pin message in a chat room. Users except UserB will still be aware of the existence of UserA.
    :::
    
    :::info
    :bulb: Although UserB can not know the chat room of UserA by `list-chat-room`, UserB can still enter the chat room of UserA by `enter-chat-room`.
    ::: -->
    
  
    
## Chat room commands

- pin
    > /pin <message>
    1. Messages should be sent by TCP packets.
    2. The length of the message can be at most 150 characters.
    3. Send `Pin -> [<username>]: <message>\n` to all users in the chatroom.
    4. Set the pin message in the chat room.
    
    <br>
    
    :::info
    :bulb: Only one message can be pinned in a chat room. The chat room only keeps the latest pinned message.
    :::

- delete pin
    > /delete-pin
    1. If there is no pin message in the chat room:
        - **`No pin message in chat room <number>`**
    2. Delete the pin message in the chat room.

    
- exit chat room
    > /exit-chat-room
    1. Switch to the chat server mode.
    2. Send the message `<username> had left the chat room.` to all clients in the chat room.

- list user
    > /list-user
    1. List all users in this chat room
        - **`<usename> <status>`**
        - **`<usename> <status>`**
        - **`...`**

- chat message
    > <message>
    1. The length of the message can be at most 150 characters.
    2. Send `[<username>]: <message>\n` to all users in the chatroom.
    
    <br>
    
    :::info
    :bulb: In our test cases, **\<message>** contains all printable ASCII characters except `\0` and `\n`.
    :::
        
:::warning
A client can use only chat room commands when entering a chat room. The client cannot use all basic commands in a chat room. Input without a leading `/` is considered a typical **message**. If a command is not specified in this document, the terminal will show `Error: Unknown command` .

:::

### Chat History
When a client joins a (`enter-chat-room`) chat room, the server should immediately encapsulate all messages in **chat history** in **`the format of <chat_history>`** and send it to the client in the **TCP connection**. The **`format of <chat_history>`** is:

```
[<username>]: <message>\n[<username>]: <message>\n...
```

### Content Filtering

To keep our chat room a joyful and peaceful space, some keywords should not be used. Below is the filtering list of those keywords. The server must mask each matched word in `<message>` sent by the clients by replacing the word with a sequence of `*` characters of the same length. 
        
**You must hard code the filtering list in your C/C++ program file.**

#### filtering list
```=
==
Superpie
hello
Starburst Stream
Domain Expansion
```

:::info
:bulb: Keyword matching is **case insensitive**.
:::

    
    
For more details about the implementation, please check the demonstration section for the sample input and the corresponding output.
    
Use ‘’% “ as the basic command line prompt. Notice that there is only one space after the prompt message. The server closes the connection for the client invoking the exit command. Nevertheless, the server keeps running in the background and accepts new incoming clients.

To run your server, you must provide the port number for your program. You can use nc or telnet command line tool to connect to your server.
    
```
server usage: ./hw2_chat_server [port number]
```

## Scoring

1. [20pts] Your server passes Example 1.
2. [20pts] Your server passes Example 2.
3. [20pts] Your server passes Example 3.
4. [40pts] Your server passes the hidden test cases announced  by TAs on January 4th.


## Remarks
* We will compile your homework by simply typing `make`. 
* Please implement your homework in C or C++.
* Using any non-standard libraries or any external binaries (e.g., via system() or other functions that invoke an external process) is not allowed.
