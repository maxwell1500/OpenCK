#ifndef SEARCHALGORITHM_H
#define SEARCHALGORITHM_H

#include "../world/ckid.hpp"
#include "../world/data.hpp"
#include "../world/collection.hpp"

#include <QString>
#include <QVector>
#include <QVariant>
#include <QRegularExpression>
#include <QDateTime>

class SearchAlgorithm
{
public:
    enum class MatchMode { Contains, StartsWith, EndsWith, Exact, Regex };
    
    struct SearchResult
    {
        QString editorId;
        QString formId;
        CkId::Type type;
        int collectionIndex;
        int recordIndex;
    };
    
    struct SearchCriteria
    {
        QString text;
        QString field; // "EditorID", "FormID", "Name", "All"
        CkId::Type typeFilter = CkId::Type_None;
        MatchMode matchMode = MatchMode::Contains;
        bool caseSensitive = false;
        
        // Multi-criteria support
        struct Criterion
        {
            QString field;
            QString value;
            MatchMode mode = MatchMode::Contains;
            bool caseSensitive = false;
        };
        QVector<Criterion> additionalCriteria;
        bool allCriteriaMustMatch = true; // AND vs OR logic
    };
    
    struct SavedSearch
    {
        QString name;
        SearchCriteria criteria;
        QDateTime created;
    };

    static QVector<SearchResult> search(const Data& data, const QString& text,
                                        const QString& field, CkId::Type typeFilter);
    
    static QVector<SearchResult> searchAdvanced(const Data& data, const SearchCriteria& criteria);
    
    static bool matchesCriteria(const QString& value, const QString& searchText, MatchMode mode, bool caseSensitive);
    static bool matchesAllCriteria(const QVector<QString>& ids, const QVector<QString>& formIds, const QVector<QString>& names,
                                   const SearchCriteria& criteria);

private:
    static QVector<SearchResult> searchCollection(
        const SearchCriteria& criteria, CkId::Type type,
        const QVector<QString>& ids, const QVector<QString>& formIds,
        const QVector<QString>& names, int collectionIndex);
};

#endif // SEARCHALGORITHM_H
