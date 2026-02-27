#ifndef MESSAGE_REPOSITORY_H
#define MESSAGE_REPOSITORY_H

#include <string>

class Database;  // Forward declaration

class MessageRepository {
public:
    explicit MessageRepository(Database& db);
    
    void addMessage(const std::string& messageContent);
    
private:
    Database& db;
};

#endif // MESSAGE_REPOSITORY_H