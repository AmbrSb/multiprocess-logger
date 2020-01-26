#pragma once

#include <functional>
#include <memory>
#include <exception>
#include <shared_mutex>

#include <arpa/inet.h>

#include <sqlite3.h>
#include <grpcpp/grpcpp.h>
#include <glog/logging.h>

#include "registry_lcl.hpp"
#include "registry/registry_service.grpc.pb.h"

namespace registry
{

class RegistrationFailed: public std::exception {};
class UnregistrationFailed: public std::exception {};
class SQLite3OpenFailed: public std::exception {};
class SQLite3InitDbFailed: public std::exception {};
class SQLite3InsertionFailed: public std::exception {};

/**
 * @brief The interface for Registry classes.
 * 
 */
class AbstractRegistry {
public:
    using Callback = std::function<void(std::vector<RegItem>)>;
    using FilterCallback = std::tuple<Filter, Callback>;

    virtual void Register(RegItem ri) noexcept = 0 ;
    virtual void Unregister(RegItem ri) noexcept = 0;
    virtual void AddCallback(Filter flt, Callback cb) noexcept = 0;
    virtual void RemoveCallback(Filter flt, Callback cb) noexcept = 0;
    virtual std::vector<RegItem> Lookup(Filter flt) noexcept = 0;
};

/**
 * @brief Maintains a list of RegItem.
 * 
 * Registry is responsible for maintaining a list of
 * RegItem. It exposes its services through a channel defined
 * by C.
 * 
 * @tparam C This is a channel that exposes to its clients
 * the services of Registry in any way it desires.
 */
template <typename C, typename I>
class RegistrySuper final: public AbstractRegistry {
#define REGISTRY_API
    public:
        RegistrySuper() noexcept;
        /**
         * @brief Constructor template.
         * 
         * This constructor template constructs a
         * communication channel of type C using the
         * parameters passed in Args.
         * 
         * @tparam Args Arguments passed to the constructor
         * of C
         */
        template <typename... Args>
        RegistrySuper(Args&&...) noexcept;
        RegistrySuper(RegistrySuper const& r) = delete;
        RegistrySuper(RegistrySuper&& r) noexcept = default;
        RegistrySuper& operator=(RegistrySuper const& r) = delete;
        RegistrySuper& operator=(RegistrySuper&& r) noexcept = default;
        ~RegistrySuper() noexcept;

        /**
         * @brief Register a new RegItem with this Registry.
         * 
         * If an equivalent RegItem is already registered in
         * this Registry, this method does not nothing and
         * returns.
         * 
         * @param ri RegItem to be added.
         */
         REGISTRY_API void Register(RegItem ri) noexcept override;
        /**
         * @brief Unregister a RegItem present in this Registry.
         * 
         * If the RegItem is not present in the registry, this
         * method does nothing and returns without any error
         * indication
         * 
         * @param ri 
         */
        REGISTRY_API void Unregister(RegItem ri) noexcept override;
        REGISTRY_API void AddCallback(Filter flt, Callback cb) noexcept override;
        REGISTRY_API void RemoveCallback(Filter flt, Callback cb) noexcept override;
        /**
         * @brief Get a list of RegItems that match a given
         * Filter.
         * 
         * @param flt The filter compared against the field
         * RegItem.name_ for all RegItem in this Registry.
         * @return std::vector<RegItem> A std::vector of all
         * matched RegItems.
         */
        REGISTRY_API std::vector<RegItem> Lookup(Filter flt) noexcept override;

        /**
         * @brief Blocks on the underlying channel C.
         * 
         * It calls the Wait() method on channel C to block
         * and wait for incomming requests.
         */
        REGISTRY_API void Wait();

    private:
        /**
         * @brief A pointer to the private implemenation
         * of Registry, implementing the PIMPL idiom.
         * XXX: currently using PIMPL here is pointless
         * due to heavy use of templates. This is for
         * future development plans.
         */
        std::unique_ptr<I> pimpl_;
        /**
         * @brief The communication channel used to access
         * this Registry.
         */
        C downstream_;
};


bool
operator==(AbstractRegistry::FilterCallback const& fc1,
           AbstractRegistry::FilterCallback const& fc2)
{
    using std::get;
    using cbtype = void(std::vector<RegItem>);

    return (get<0>(fc1) == get<0>(fc2)) &&
        (get<1>(fc1).target<cbtype>() == get<1>(fc2).target<cbtype>());
}

/**
 * @brief Private implementation for Registry.
 * 
 */
class RegistryImplVec {
public:
    inline void Register(RegItem ri);
    inline void Unregister(RegItem ri);
    inline void AddCallback(Filter flt, AbstractRegistry::Callback cb);
    inline void RemoveCallback(Filter flt, AbstractRegistry::Callback cb);
    inline std::vector<RegItem> Lookup(Filter flt);

private:
    std::vector<RegItem> items_;
    std::vector<AbstractRegistry::FilterCallback> callbacks_;
    std::shared_mutex mutable items_lock_;
    std::shared_mutex mutable callbacks_lock_;

    std::vector<RegItem> CheckFilters(void);
    std::vector<RegItem> CheckFilters(Filter const flt);
    int CheckCallbacks(void);
    int CheckCallbacks(AbstractRegistry::FilterCallback const& fcb);
    int CheckCallbacks(RegItem const& ri);
};

inline void
RegistryImplVec::Register(RegItem ri)
{
    using std::begin;
    using std::end;
    using std::find_if;

    std::unique_lock{items_lock_};
    auto it = find(begin(items_), end(items_), ri);

    if (it == end(items_))
        items_.push_back(ri);
    CheckCallbacks();
}

inline void
RegistryImplVec::Unregister(RegItem ri)
{
    using std::begin;
    using std::end;
    using std::find_if;

    std::unique_lock{items_lock_};
    auto it = find(begin(items_), end(items_), ri);
    if (it != end(items_))
        items_.erase(it);
    CheckCallbacks();
}

inline void
RegistryImplVec::AddCallback(Filter flt, AbstractRegistry::Callback cb)
{
    std::unique_lock{callbacks_lock_};
    auto fcb = callbacks_.emplace_back(flt, cb);
    CheckCallbacks(fcb);
}

inline void
RegistryImplVec::RemoveCallback(Filter flt, AbstractRegistry::Callback cb)
{
    using std::begin;
    using std::end;
    using std::find;

    auto fc = std::make_tuple(flt, cb);
    std::unique_lock{callbacks_lock_};
    auto it = std::find(begin(callbacks_), end(callbacks_), fc);
    callbacks_.erase(it);
}

inline std::vector<RegItem>
RegistryImplVec::Lookup(Filter flt)
{
    return CheckFilters(flt);
}

int
RegistryImplVec::CheckCallbacks(AbstractRegistry::FilterCallback const& fcb)
{
    using std::begin;
    using std::end;
    using std::find;

    std::vector<RegItem> matches;
    auto oi = std::back_inserter(matches);
    auto flt = std::get<0>(fcb);
    auto cb = std::get<1>(fcb);

    std::shared_lock{items_lock_};
    std::copy_if(begin(items_),
                 end(items_),
                 oi,
                 [&flt](auto const& r){ return flt(r.GetName()); });
    cb(matches);
    return std::size(matches);
}

int
RegistryImplVec::CheckCallbacks(RegItem const& ri)
{
    int count = 0;

    std::shared_lock{callbacks_lock_};
    for (auto const& fcb : callbacks_) {
        auto flt = std::get<0>(fcb);
        if (flt(ri.GetName())) {
            auto cb = std::get<1>(fcb);
            // XXX: optimize this. we do not need a vec here!
            cb(std::vector{ri});
            count++;
        }
    }
    return count;
}

int
RegistryImplVec::CheckCallbacks()
{
    using std::begin;
    using std::end;
    using std::for_each;

    int count = 0;

    std::shared_lock{callbacks_lock_};
    for (auto const& cb : callbacks_)
        count += CheckCallbacks(cb);
    return count;
}

std::vector<RegItem>
RegistryImplVec::CheckFilters(Filter const flt)
{
    using std::begin;
    using std::end;
    using std::find;
    std::vector<RegItem> matches;
    auto oi = std::back_inserter(matches);
    std::shared_lock{items_lock_};
    std::copy_if(begin(items_),
                 end(items_),
                 oi,
                 [&flt](auto const& r){ return flt(r.GetName()); });
    return matches;
}

std::vector<RegItem>
RegistryImplVec::CheckFilters(void)
{
    using std::begin;
    using std::end;
    using std::for_each;

    std::vector<RegItem> matches;
    auto mi = std::back_insert_iterator(matches);

    std::shared_lock{callbacks_lock_};
    std::for_each(begin(callbacks_),
                  end  (callbacks_),
                  /* Find all matching RegItems for each Callback */
                  [this, &mi](auto const& fcb){
                      auto const& more_items = this->CheckFilters(std::get<0>(fcb));
                      std::move(begin(more_items), end(more_items), mi);
                  });
    return matches;
}

class RegistryImplSQLite {
public:
    inline RegistryImplSQLite(std::string db_path);
    ~RegistryImplSQLite();
    inline void Register(RegItem ri);
    inline void Unregister(RegItem ri);
    inline void AddCallback(Filter flt, AbstractRegistry::Callback cb);
    inline void RemoveCallback(Filter flt, AbstractRegistry::Callback cb);
    inline std::vector<RegItem> Lookup(Filter flt);

private:
    sqlite3* db_;
    std::string const kItemsTableName = "ITEMS";
    std::vector<AbstractRegistry::FilterCallback> callbacks_;
    std::shared_mutex mutable items_lock_;
    std::shared_mutex mutable callbacks_lock_;

    std::vector<RegItem> CheckFilters(void);
    std::vector<RegItem> CheckFilters(Filter const flt);
    int CheckCallbacks(void);
    int CheckCallbacks(AbstractRegistry::FilterCallback const& fcb);
    int CheckCallbacks(RegItem const& ri);
    void InitDb();
    bool CheckDb();
    bool SqlSelectInDb(std::string criteria, std::vector<RegItem>& matches);
};

bool
RegistryImplSQLite::CheckDb()
{
    std::string check_query = "SELECT * FROM " + kItemsTableName + ";";
    return SQLITE_OK == sqlite3_exec(db_,
                                     check_query.c_str(),
                                     NULL, NULL, NULL); 
}

void
RegistryImplSQLite::InitDb()
{
    char* zErrMsg;

    if (!CheckDb()) {
        LOG(INFO) << "Initializing DB...";
        LOG(INFO) << "Creating table `" << kItemsTableName << "`.";
        std::string query =                                             \
                "CREATE TABLE " + kItemsTableName + " ("                \
                "NAME   TEXT                              NOT NULL,"    \
                "LOCA   TEXT                              NOT NULL,"    \
                "PRIMARY KEY('NAME', 'LOCA'));";
        int rc = sqlite3_exec(db_, query.c_str(), NULL, 0, &zErrMsg);
        if (SQLITE_OK != rc) {
            LOG(ERROR) << "Can't create table " << kItemsTableName << ": " << sqlite3_errmsg(db_);
            throw SQLite3InitDbFailed{};
        } else {
            LOG(INFO) << "Table ITEMS created successfully";
        }
    }
}

inline
RegistryImplSQLite::RegistryImplSQLite(std::string db_path = "registry.sqlite")
{
    int rc = sqlite3_open(db_path.c_str(), &db_);
    if (rc != SQLITE_OK) {
        LOG(ERROR) << "Can't open database " << db_path << ": " << sqlite3_errmsg(db_);
        throw SQLite3OpenFailed{};
    } else {
        LOG(INFO) << "Opened database " << db_path << " successfully";
    }
    InitDb();
}

RegistryImplSQLite::~RegistryImplSQLite()
{
    if (db_)
        sqlite3_close(db_);
}

inline void
RegistryImplSQLite::Register(RegItem ri)
{
    std::unique_lock{items_lock_};

    int rc;
    sqlite3_stmt* sql_stmt;
    char const* pzTail;

    std::string query = "INSERT OR IGNORE INTO " + kItemsTableName + "  (NAME, LOCA) "    \
                        "VALUES (?,?);"; 
    rc = sqlite3_prepare(db_, query.c_str(), 1024, &sql_stmt, &pzTail);
    rc = sqlite3_bind_text(sql_stmt,
                           1,
                           ri.GetName().c_str(),
                           ri.GetName().size(),
                           SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        LOG(ERROR) << "Can't insert into table " << kItemsTableName << ": " << sqlite3_errmsg(db_);
        throw SQLite3InsertionFailed{};
    }
    rc = sqlite3_bind_text(sql_stmt,
                           2,
                           ri.GetLocation().name.c_str(),
                           ri.GetLocation().name.size(),
                           SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        LOG(ERROR) << "Can't insert into table " << kItemsTableName << ": " << sqlite3_errmsg(db_);
        throw SQLite3InsertionFailed{};
    }
    rc = sqlite3_step(sql_stmt);
    if (rc != SQLITE_DONE) {
        LOG(ERROR) << "Can't insert into table " << kItemsTableName << ": " << sqlite3_errmsg(db_);
        throw SQLite3InsertionFailed{};
    }
    CheckCallbacks();
}

inline void
RegistryImplSQLite::Unregister(RegItem ri)
{
    std::unique_lock{items_lock_};

    int rc;
    sqlite3_stmt* sql_stmt;
    char const* pzTail;

    std::string query = "DELETE FROM " + kItemsTableName + " WHERE NAME = ? AND LOCA = ?;"; 
    rc = sqlite3_prepare(db_, query.c_str(), 1024, &sql_stmt, &pzTail);
    rc = sqlite3_bind_text(sql_stmt,
                           1,
                           ri.GetName().c_str(),
                           ri.GetName().size(),
                           SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        LOG(ERROR) << "Can't insert into table " << kItemsTableName << ": " << sqlite3_errmsg(db_);
        throw SQLite3InsertionFailed{};
    }
    rc = sqlite3_bind_text(sql_stmt,
                           2,
                           ri.GetLocation().name.c_str(),
                           ri.GetLocation().name.size(),
                           SQLITE_STATIC);
    rc = sqlite3_step(sql_stmt);
    if (rc != SQLITE_DONE) {
        LOG(ERROR) << "Can't delete from table " << kItemsTableName << ": " << sqlite3_errmsg(db_);
        throw SQLite3InsertionFailed{};
    }
    CheckCallbacks();
}

inline void
RegistryImplSQLite::AddCallback(Filter flt, AbstractRegistry::Callback cb)
{
    std::unique_lock{callbacks_lock_};
    auto fcb = callbacks_.emplace_back(flt, cb);
    CheckCallbacks(fcb);
}

inline void
RegistryImplSQLite::RemoveCallback(Filter flt, AbstractRegistry::Callback cb)
{
    using std::begin;
    using std::end;
    using std::find;

    auto fc = std::make_tuple(flt, cb);
    std::unique_lock{callbacks_lock_};
    auto it = std::find(begin(callbacks_), end(callbacks_), fc);
    callbacks_.erase(it);
}

inline std::vector<RegItem>
RegistryImplSQLite::Lookup(Filter flt)
{
    return CheckFilters(flt);
}

bool
RegistryImplSQLite::SqlSelectInDb(std::string criteria, std::vector<RegItem>& matches)
{
    int rc;
    sqlite3_stmt* sql_stmt;
    char const* pzTail;

    std::string query = "SELECT * FROM " + kItemsTableName + " WHERE NAME = ?;"; 
    rc = sqlite3_prepare(db_, query.c_str(), 1024, &sql_stmt, &pzTail);
    rc = sqlite3_bind_text(sql_stmt,
                           1,
                           criteria.c_str(),
                           criteria.size(),
                           SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        LOG(ERROR) << "Can't create filter query for table "
                   << kItemsTableName << ": " << sqlite3_errmsg(db_);
        throw SQLite3InsertionFailed{};
    }

    while ((rc = sqlite3_step(sql_stmt)) == SQLITE_ROW) {
        char const* name = (char const*)sqlite3_column_text(sql_stmt, 0);
        char const* loca = (char const*)sqlite3_column_text(sql_stmt, 1);
        matches.push_back(
            RegItem{std::string{name},
                    BufferLocation{std::string{loca}}});
    }
    if (rc != SQLITE_DONE) {
        LOG(ERROR) << "Can't search filter in table "
                   << kItemsTableName << ": " << sqlite3_errmsg(db_);
                   return false;
    }
    return true;
}

int
RegistryImplSQLite::CheckCallbacks(AbstractRegistry::FilterCallback const& fcb)
{
    std::vector<RegItem> matches;
    auto flt = std::get<0>(fcb);
    auto cb = std::get<1>(fcb);

    if (SqlSelectInDb(flt.GetFilterText(), matches))
        cb(matches);
    else
        throw SQLite3InsertionFailed{};

    return std::size(matches);
}

int
RegistryImplSQLite::CheckCallbacks(RegItem const& ri)
{
    int count = 0;

    std::shared_lock{callbacks_lock_};
    for (auto const& fcb : callbacks_) {
        auto flt = std::get<0>(fcb);
        if (flt(ri.GetName())) {
            auto cb = std::get<1>(fcb);
            // XXX: optimize this. we do not need a vec here!
            cb(std::vector{ri});
            count++;
        }
    }
    return count;
}

int
RegistryImplSQLite::CheckCallbacks()
{
    using std::begin;
    using std::end;
    using std::for_each;

    int count = 0;

    std::shared_lock{callbacks_lock_};
    for (auto const& cb : callbacks_)
        count += CheckCallbacks(cb);
    return count;
}

std::vector<RegItem>
RegistryImplSQLite::CheckFilters(Filter const flt)
{
    using std::begin;
    using std::end;
    using std::find;
    std::vector<RegItem> matches;
    std::shared_lock{items_lock_};

    int rc;
    sqlite3_stmt* sql_stmt;
    char const* pzTail;

    std::string query = "SELECT * FROM " + kItemsTableName + " WHERE NAME = ?;"; 
    rc = sqlite3_prepare(db_, query.c_str(), 1024, &sql_stmt, &pzTail);
    rc = sqlite3_bind_text(sql_stmt,
                           1,
                           flt.GetFilterText().c_str(),
                           flt.GetFilterText().size(),
                           SQLITE_STATIC);
    if (rc != SQLITE_OK) {
        LOG(ERROR) << "Can't create filter query for table "
                   << kItemsTableName << ": " << sqlite3_errmsg(db_);
        throw SQLite3InsertionFailed{};
    }

    while ((rc = sqlite3_step(sql_stmt)) == SQLITE_ROW) {
        char const* name = (char const*)sqlite3_column_text(sql_stmt, 0);
        char const* loca = (char const*)sqlite3_column_text(sql_stmt, 1);
        matches.push_back(
            RegItem{
                std::string{name},
                BufferLocation{std::string{loca}}});
    }
    if (rc != SQLITE_DONE) {
        LOG(ERROR) << "Can't search filter in table ITEMS: " << sqlite3_errmsg(db_);
        throw SQLite3InsertionFailed{};
    }

    return matches;
}

std::vector<RegItem>
RegistryImplSQLite::CheckFilters(void)
{
    using std::begin;
    using std::end;
    using std::for_each;

    std::vector<RegItem> matches;
    auto mi = std::back_insert_iterator(matches);

    std::shared_lock{callbacks_lock_};
    std::for_each(begin(callbacks_),
                  end  (callbacks_),
                  /* Find all matching RegItems for each Callback */
                  [this, &mi](auto const& fcb){
                      auto const& more_items = this->CheckFilters(std::get<0>(fcb));
                      std::move(begin(more_items), end(more_items), mi);
                  });
    return matches;
}


template<typename C>
using Registry = RegistrySuper<C, RegistryImplVec>;
template<typename C>
using RegistryDB = RegistrySuper<C, RegistryImplSQLite>;

template <typename C, typename Impl>
RegistrySuper<C, Impl>::RegistrySuper() noexcept
    : pimpl_{new Impl},
      downstream_{}
{
}

template <typename C, typename Impl>
template <typename... Args>
RegistrySuper<C, Impl>::RegistrySuper(Args&&... args) noexcept
    : pimpl_{new Impl},
      downstream_{std::forward<Args...>(args...), this}
{
}

template <typename C, typename Impl>
RegistrySuper<C, Impl>::~RegistrySuper() noexcept = default;

template <typename C, typename Impl>
void
RegistrySuper<C, Impl>::Wait()
{
    downstream_.Wait();
}

template <typename C, typename Impl>
void
RegistrySuper<C, Impl>::Register(RegItem ri) noexcept
{
    pimpl_->Register(ri);
}

template <typename C, typename Impl>
void
RegistrySuper<C, Impl>::Unregister(RegItem ri) noexcept
{
    pimpl_->Unregister(ri);
}

template <typename C, typename Impl>
void
RegistrySuper<C, Impl>::AddCallback(Filter flt,
                                    AbstractRegistry::Callback cb) noexcept
{
    pimpl_->AddCallback(flt, cb);
}

template <typename C, typename Impl>
void
RegistrySuper<C, Impl>::RemoveCallback(Filter flt,
                                       AbstractRegistry::Callback cb) noexcept
{
    pimpl_->RemoveCallback(flt, cb);
}

template <typename C, typename Impl>
std::vector<RegItem>
RegistrySuper<C, Impl>::Lookup(Filter flt) noexcept
{
    return pimpl_->Lookup(flt);
}

/**
 * @brief gRPC service implementation.
 * 
 * This classes directly uses the services of the Registry
 * class to respond to RPC request.
 * 
 */
class ComService final
    : public RegistryService::Service {
private:
    /**
     * @brief An instance of Registry to be used to carry out
     * the requested operations.
     * 
     */
    AbstractRegistry* upstream_;

public:
    ComService(AbstractRegistry* ar)
        : upstream_{ar} {}

private:
    grpc::Status
    Register(grpc::ServerContext* cxt,
             ComMsg const* msg,
             Result* rslt) override
    {
        auto items = msg->reg_item();
        if (items.size() == 0)
            return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT,
                                "No RegItems were received."};
        for (auto& item : items)
            upstream_->Register(RegItem{item});
        return grpc::Status::OK;
    }

    grpc::Status
    Unregister(grpc::ServerContext* cxt,
               ComMsg const* msg,
               Result* rslt) override
    {
        auto items = msg->reg_item();
        if (items.size() == 0)
            return grpc::Status{grpc::StatusCode::INVALID_ARGUMENT,
                                "No RegItems were received."};
        for (auto& item : items)
            upstream_->Unregister(RegItem{item});
        return grpc::Status::OK;
    }

    grpc::Status
    Lookup(grpc::ServerContext* cxt,
           ComMsg const* msg,
           Result* rslt) override
    {
        auto items = upstream_->Lookup(Filter{msg->fltr()});
        for (auto const& itm : items) {
            auto x = rslt->add_reg_item();
            if (!x)
                return grpc::Status{grpc::StatusCode::RESOURCE_EXHAUSTED,
                                    "Could allocate memory for gRPC message."};
            x->set_name(itm.GetName());
            x->set_location(itm.GetLocation().name);
        }
        rslt->set_code(std::size(items));
        rslt->set_error_message("Success");
        return grpc::Status::OK;
    }

    grpc::Status
    AddCallback(grpc::ServerContext* cxt,
                ComMsg const* msg,
                Result* rslt) override
    {
        return grpc::Status{grpc::StatusCode::DO_NOT_USE,
                            "Not implemented."};
    }

    grpc::Status
    RemoveCallback(grpc::ServerContext* cxt,
                   ComMsg const* msg,
                   Result* rslt) override
    {
        return grpc::Status{grpc::StatusCode::DO_NOT_USE,
                            "Not implemented."};
    }

};

using grpc::Server;
using grpc::ServerBuilder;
using grpc::Channel;
using grpc::ClientContext;

/**
 * @brief Proxy for ComService used by Registry.
 * 
 * Registry can use this class to make a new service proxy
 * to expose its services through any means possible by the
 * ServiceComChannel.
 * 
 */
class ServerComChannel {
    public:
        ServerComChannel(RegistryLocation const& l, AbstractRegistry * ar) noexcept
            : peerloc_{l},
              service_{ComService{ar}},
              server_{nullptr} {
                    ServerBuilder builder;
                    builder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_GZIP);
                    builder.AddListeningPort(l, grpc::InsecureServerCredentials());
                    builder.RegisterService(&service_);
                    server_ = builder.BuildAndStart();
                    std::cout << "Server listening on "
                              << static_cast<std::string>(l)
                              << std::endl;
              }
        ServerComChannel(ServerComChannel const&) = delete;
        ServerComChannel(ServerComChannel const&&) = delete;
        ServerComChannel& operator==(ServerComChannel const&) = delete;
        ServerComChannel& operator==(ServerComChannel const&&) = delete;
        ~ServerComChannel() noexcept = default;

        /**
         * @brief Block on the underlying channel and respond
         * to its requests.
         * 
         */
        void Wait() const {
            server_->Wait();
        }

    private:
        /**
         * @brief Location of the registry.
         */
        RegistryLocation peerloc_;
        ComService service_;
        /**
         * @brief gRPC server instance.
         */
        std::unique_ptr<Server> server_;
};

/**
 * @brief This is a proxy that allows Registry clients to
 * use its services remotly. This is actually a wrapper
 * for the gRPC service stub.
 * 
 */
class ClientComChannel {
    public:
        ClientComChannel(RegistryLocation const& l) noexcept
            : peerloc_{l},
              stub_{RegistryService::NewStub(
                    grpc::CreateChannel(l, grpc::InsecureChannelCredentials()))}
              {
              }
        ClientComChannel(ClientComChannel const&) = delete;
        ClientComChannel(ClientComChannel const&&) = delete;
        ClientComChannel& operator=(ClientComChannel const&) = delete;
        ClientComChannel& operator=(ClientComChannel const&&) = delete;
        ~ClientComChannel() noexcept = default;

        std::vector<RegItem> Lookup(Filter const& filter) const {
            std::vector<RegItem> items;
            Result result;
            ComMsg msg;
            Fltr* fltr = new Fltr;
            fltr->set_definition(filter.GetFilterText());
            msg.set_allocated_fltr(fltr);
            fltr = nullptr;
            ClientContext context;
            grpc::Status status = stub_->Lookup(&context, msg, &result);
            if (!status.ok())
                throw LookupFailed{};
            for (int i = 0; i < result.reg_item_size(); i++) {
                auto r = result.reg_item(i);
                RegItem rgitem{r.name(), r.location()};
                items.push_back(rgitem);
            }
            return items;
        }

        void Register(RegItem const& reg_item) const {
            Result result;
            ComMsg msg;
            auto rgitm = msg.add_reg_item();
            rgitm->set_name(reg_item.GetName());
            rgitm->set_location(reg_item.GetLocation().name);
            ClientContext context;
            grpc::Status status = stub_->Register(&context, msg, &result);
            if (!status.ok())
                throw RegistrationFailed{};
        }

        void Unregister(RegItem const& reg_item) const {
            Result result;
            ComMsg msg;
            auto rgitm = msg.add_reg_item();
            rgitm->set_name(reg_item.GetName());
            rgitm->set_location(reg_item.GetLocation().name);
            ClientContext context;
            grpc::Status status = stub_->Unregister(&context, msg, &result);
            if (!status.ok())
                throw UnregistrationFailed{};
        }

    private:
        /**
         * @brief Location of the registry.
         */
        RegistryLocation peerloc_;
        /**
         * @brief Client-side gRPC service stub.
         * 
         */
        std::unique_ptr<RegistryService::Stub> stub_;
};
}
