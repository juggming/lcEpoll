#### lcepoll框架
 * 以为linux epoll为基础实现的基本框架
 * `pool.c`: 简单的线程池实现，用于管理连接
 * `network.c`: 封装了常用socket选项，比如设置非阻塞socket、TCP_NODELAY、KEEPALIVE等
 * `lcepollc`: 框架的主体结构代码
 * `ring.c`: 实现了环形数组
 * `event.c`: 事件管理,定时器
 * `hash_table.c`:以Google开源的murmur2实现的hash
