# BankTerminalProject
Databases Course Project

Copyright 2020 Louis Duller

This project required the creation of a bank terminal application. The project goal was to access students' comprehension and application of database concepts and capabilities in a practical environment. The application was required to have a menu, let the user create new accounts, close accounts, deposit, withdraw, and transfer funds. Students were to setup a relational database for the application to connect to, and write the application to meet the functional requirements above. Deposits, withdrawals, and transfers were to make use of table locking and checkpoints to meet concurrency and consistency requirements, as well. Students had to use MySQL for the database environment, but could choose any programming language they could find a MySQL connector for.

The choice of C++ made this a more difficult project than it might have been using Python or some other language, but the challenges presented by this choice made this a great learning experience. The MySQL C++ Connector had just recently undergone an update; deciphering the documentation for something as simple as a query was the first of the challenges. Another challenge was setting up the build environment - I used the dynamic library as a way to learn how to set project build options for these library types. Unfortunately, there were still some challenges I did not overcome that limit the usefulness of the application as noted in the .cpp file.
