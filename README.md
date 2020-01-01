# SDN
Assigned school project, all functionally with the exception of Thread classes, parseOptions and usage were implemented by me. 
The point of this project is to be a scaled down version of netcat and as such provides similar functionality and is very similar to what is found on the linux man pages for netcat. I learned alot of this code from http://beej.us/guide/bgnet/ a great guide on network programming. Theres still alot of error handling I have left to implement to make this perfect but it is OTW!Its a really finnicy version of linux that requires it, its the one that runs on dept servers GNU Make 4.2.1 Built for x86_64-suse-linux-gnu, but apparently its free so you can get it if you want. 


Examples how to use 
(Use make to compile) 
nc www.test.dict.org 2168 would open a tcp conection to post 42 of www.test.dict.org 2168
nc -l 3456 would make the server listen for a connection on port 3456 
-k mean sthe server will continue to listen even when no on else is connected (Supports 10 connections at once) 

