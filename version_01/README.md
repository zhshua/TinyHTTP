仿照tinyhttpd项目写的，每个函数的实现可能有一些不一样，不过大体的函数功能是一样的
### 阅读代码的顺序为：
main()——>startup()——>accept_request()——>serve_file()——>execute_cig()。

### 流程图如下：
![流程图](https://img-blog.csdnimg.cn/20210223222345257.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3pzMTIwMTk3,size_16,color_FFFFFF,t_70#pic_center)
### cgi模块执行原理
![cgi执行原理](https://img-blog.csdnimg.cn/20210223224152150.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L3pzMTIwMTk3,size_16,color_FFFFFF,t_70#pic_center)
图例解释：定义两个管道input，output，

input：由父进程向子进程的输入管道

output：由子进程向父进程的输出管道

1、让子进程去执行cgi，父进程去做I/O操作。首先将子进程的标准输入重定向为input[0]，将子进程的标准输出重定向为output[1]，这样，子进程的输入来自父进程，子进程的输出是输出到父进程。通过管道进行父子进程的通信。

2、父进程通过recv接收来自客户端的表单输入，并且通过write传给子进程，子进程收到父进程的输入后，执行cgi脚本，执行完后，输出给父进程。父进程通过read读取到子进程的输出后，再通过send发送到客户端。


### 遇到的问题
在写完程序后，启动服务器，客户端能正确收到初始的index.html页面，当在表单里输入数据并且提交后，却显示不出来请求后的颜色页面，可能的原因有如下几个：

1、color.cgi脚本没有执行权限，通过```chmod +x color.cgi```命令，给该文件添加执行权限。

2、index.html没有写的权限，通过```chmod 600 index.html```设置权限。

3、perl安装的路径不对，perl默认的安装路径是/usr/bin/perl，将color.cgi文件第一行的```#!/usr/local/bin/perl -Tw```修改为``#!/usr/bin/perl -Tw``

