#pragma once
#include "sqlite3.h"

class CSqliteStmt;
class CSqliteBackup;
class CSqlite
{
private:
	sqlite3 *db;
	CSqlite(const CSqlite& one){}
	CSqlite& operator=(const CSqlite& one){return *this;}
public:
	inline CSqlite(CSqlite&& one)
	{
		db=one.db;
		one.db=nullptr;
	}
	inline CSqlite& operator=(CSqlite&& one)
	{
		if(one.db!=nullptr)
		{
			Close();
			db=one.db;
			one.db=nullptr;
		}
		return *this;
	}
	inline const char *getLastErrorMsg(){return sqlite3_errmsg(db);}
	inline void EnableSharedCache(int enable){sqlite3_enable_shared_cache(enable);}
	inline CSqlite():db(nullptr){}
	inline ~CSqlite(){Close();}
	inline int Close()
	{
		int res=SQLITE_OK;
		if(db!=nullptr)
		{
			res=sqlite3_close(db);
			db=nullptr;
		}
		return res;
	}
	inline int Open(const wchar_t* file,int flags=SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)
	{return sqlite3_open_v2(CW2AEX<>(file,CP_UTF8), &db ,flags,NULL);}
	inline int WalAutoCheckPoint(int N){return sqlite3_wal_autocheckpoint(db,N);}
	inline int WalCheckPoint(const char* dbname=nullptr){return ::sqlite3_wal_checkpoint(db,dbname);}
	inline int Execute(const wchar_t* command){return sqlite3_exec(db,CW2AEX<>(command,CP_UTF8),nullptr,nullptr,NULL);}
	inline CSqliteStmt Prepare(const wchar_t *command);
	inline CSqliteBackup InitBackup(CSqlite &dest);
};

class CSqliteStmt
{
	friend class CSqlite;
private:
	sqlite3_stmt* stmt;
	CSqliteStmt(const CSqliteStmt& one){}
	CSqliteStmt& operator=(const CSqliteStmt& one){return *this;}
	inline char* Utf8Str(const wchar_t* str,int &reslen)
	{
		reslen = ::WideCharToMultiByte( CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL );
		char* temp=(LPSTR)malloc(reslen);
		::WideCharToMultiByte( CP_UTF8, 0, str, -1, temp, reslen, NULL, NULL );
		return temp;
	}
public:
	bool IsNull(){return stmt==nullptr;}
	inline CSqliteStmt():stmt(nullptr){}
	~CSqliteStmt(){Close();}
	template<typename TYPE>
	inline int Bind(const wchar_t* param,TYPE value)
	{
		int index=BindParamIndex(param);
		if(index==0) return SQLITE_ERROR;
		return Bind(index,value);
	}
	template<typename TYPE>
	inline int Bind(const char* param,TYPE value)
	{
		int index=BindParamIndex(param);
		if(index==0) return SQLITE_ERROR;
		return Bind(index,value);
	}
	inline CSqliteStmt(CSqliteStmt &&one)
	{
		stmt=one.stmt;
		one.stmt=nullptr;
	}
	inline int Close()
	{
		int rc=SQLITE_OK;
		if(stmt!=nullptr)
		{
			rc=sqlite3_finalize(stmt);
			stmt=nullptr;
		}
		return rc;
	}
	inline CSqliteStmt& operator=(CSqliteStmt&& one)
	{
		if(one.stmt!=nullptr)
		{
			Close();
			stmt=one.stmt;
			one.stmt=nullptr;
		}
		return *this;
	}
	inline int Reset()
	{
		return sqlite3_reset(stmt);
	}
	inline int Step()
	{
		return sqlite3_step(stmt);
	}
	inline int BindParamIndex(const wchar_t* param)
	{
		return sqlite3_bind_parameter_index(stmt,CW2AEX<>(param,CP_UTF8));
	}
	inline int BindParamIndex(const char* param)
	{
		return sqlite3_bind_parameter_index(stmt,param);
	}
	inline int Bind(int index,const wchar_t* value)
	{
		if(index>sqlite3_bind_parameter_count(stmt)) return SQLITE_ERROR;
		char* sv=nullptr;
		int len=0;
		sv=Utf8Str(value,len);
		int res=sqlite3_bind_text(stmt,index,sv,len,free);
		if(res!=SQLITE_OK)
			free(sv);
		return res;
	}


	inline int Bind(int index,void* value,int size,void(*freefun)(void*)=SQLITE_TRANSIENT)
	{
		return sqlite3_bind_blob(stmt, index, value, size, freefun);
	}
	inline int Bind(const wchar_t* param,void* value,int size,void(*freefun)(void*)=SQLITE_TRANSIENT)
	{
		int index=BindParamIndex(param);
		if(index==0) return SQLITE_ERROR;
		return sqlite3_bind_blob(stmt, index, value, size, freefun);
	}
	inline int Bind(const char* param,void* value,int size,void(*freefun)(void*)=SQLITE_TRANSIENT)
	{
		int index=BindParamIndex(param);
		if(index==0) return SQLITE_ERROR;
		return sqlite3_bind_blob(stmt, index, value, size, freefun);
	}
	inline int Bind(int index,double value)
	{
		return sqlite3_bind_double(stmt,index,value);
	}
	inline int Bind(int index,int value)
	{
		return sqlite3_bind_int(stmt,index,value);
	}
	inline int Bind(int index,sqlite3_int64 value)
	{
		return sqlite3_bind_int64(stmt,index,value);
	}
	inline int BindNull(int index)
	{
		return sqlite3_bind_null(stmt,index);
	}
	inline int BindNull(const wchar_t* param)
	{
		int index=BindParamIndex(param);
		if(index==0) return SQLITE_ERROR;
		return sqlite3_bind_null(stmt,index);
	}

	inline int GetBytes(int iCol){return sqlite3_column_bytes(stmt,iCol);}
	inline const void* GetBlob(int iCol){return sqlite3_column_blob(stmt,iCol);}
	inline double GetDouble(int iCol){return sqlite3_column_double(stmt,iCol);}
	inline int GetInt(int iCol){return sqlite3_column_int(stmt,iCol);}
	inline sqlite3_int64 GetInt64(int iCol){return sqlite3_column_int64(stmt,iCol);}
	inline const char* GetText(int iCol){return (const char*)sqlite3_column_text(stmt,iCol);}
	inline int GetColType(int iCol){return sqlite3_column_type(stmt,iCol);}
	inline const char* GetColName(int iCol){return sqlite3_column_name(stmt,iCol);}

	inline void ReadToEnd(){while(Step()==SQLITE_ROW);}
};

class CSqliteBackup
{
	friend class CSqlite;
private:
	sqlite3_backup * backup;
	inline CSqliteBackup(const CSqliteBackup& one){}
	inline CSqliteBackup& operator=(const CSqliteBackup& one){return *this;}
	inline CSqliteBackup():backup(nullptr){}
public:
	inline bool IsNull(){return backup==nullptr;}
	inline ~CSqliteBackup(){Close();}
	inline CSqliteBackup(CSqliteBackup&& one)
	{
		backup=one.backup;
		one.backup=nullptr;
	}
	inline int Close()
	{
		int res=SQLITE_OK;
		if(backup!=nullptr)
		{
			res=sqlite3_backup_finish(backup);
			backup=nullptr;
		}
		return res;
	}
	inline CSqliteBackup& operator=(CSqliteBackup&& one)
	{
		if(one.backup!=nullptr)
		{
			Close();
			backup=one.backup;
			one.backup=nullptr;
		}
		return *this;
	}
	inline int Step(int nPage=-1)
	{
		return sqlite3_backup_step(backup,nPage);
	}
	inline int PageCount(){return sqlite3_backup_pagecount(backup);}
	inline int Remaining(){return sqlite3_backup_remaining(backup);}
};

CSqliteStmt CSqlite::Prepare(const wchar_t *command)
{
	CSqliteStmt onestmt;
	int rc=sqlite3_prepare_v2(db,CW2A(command,CP_UTF8),-1,&onestmt.stmt,nullptr);
	ATLASSERT(rc==SQLITE_OK);
	return onestmt;
}
CSqliteBackup CSqlite::InitBackup(CSqlite &dest)
{
	CSqliteBackup backup;
	backup.backup=sqlite3_backup_init(dest.db,"main",db,"main");
	return backup;
}