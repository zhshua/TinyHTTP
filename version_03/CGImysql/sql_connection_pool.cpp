#include "sql_connection_pool.h"

connection_pool* connection_pool::GetInstance(){
    static connection_pool instance;
    return &instance;
}

MYSQL* connection_pool::GetConnection()
{
    MYSQL *con = NULL;

	if (0 == connList.size())
		return NULL;

	reserve.wait();
	
	mutex.lock();

	con = connList.front();
	connList.pop_front();

	--FreeConn;
	++CurConn;

	mutex.unlock();
	return con;
}

bool connection_pool::ReleaseConnection(MYSQL *conn)
{
    if (NULL == conn)
		return false;

	mutex.lock();

	connList.push_back(conn);
	++FreeConn;
	--CurConn;

	mutex.unlock();

	reserve.post();
	return true;
}

int connection_pool::GetFreeConn()
{
    return this->FreeConn;
}
void connection_pool::DestroyPool()
{
    mutex.lock();
	if (connList.size() > 0)
	{
		list<MYSQL *>::iterator it;
		for (it = connList.begin(); it != connList.end(); ++it)
		{
			MYSQL *con = *it;
			mysql_close(con);
		}
		CurConn = 0;
		FreeConn = 0;
		connList.clear();
	}

	mutex.unlock();
}
void connection_pool::init(string ip, string User, string PassWord, string DataBaseName, int Port, unsigned int MaxConn)
{
    this->ip = ip;
    this->User = User;
    this->PassWord = PassWord;
    this->DateBase = DateBase;
    this->Port = Port;
    this->MaxConn = MaxConn;

    mutex.lock();
    for(int i = 0;i < MaxConn;i++)
    {
        MYSQL *con = NULL;
        con = mysql_init(con);

        if (con == NULL)
        {
            cout << "Error:" << mysql_error(con);
            exit(1);
        }
        con = mysql_real_connect(con, ip.c_str(), User.c_str(), PassWord.c_str(), DateBase.c_str(), Port, NULL, 0);

        if (con == NULL)
        {
            cout << "Error: " << mysql_error(con);
            exit(1);
        }
        connList.push_back(con);
        ++FreeConn;
    }
    reserve = sem(FreeConn);
    this->MaxConn = FreeConn;   // 刚开始所有连接都是空闲的
    mutex.unlock();
}

connectionRAII::connectionRAII(MYSQL **SQL, connection_pool *connPool){
	*SQL = connPool->GetConnection();
	
	conRAII = *SQL;
	poolRAII = connPool;
}

connectionRAII::~connectionRAII(){
	poolRAII->ReleaseConnection(conRAII);
}
