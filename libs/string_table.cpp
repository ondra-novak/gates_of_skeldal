#include "string_table.h"
#include "csv.hpp"
#include "cztable.h"
#include <unordered_map>
#include <fstream>
#include <memory>


typedef struct stringtable_struct_tag {

    std::unordered_map<int, std::string> _strings;

}TSTRINGTABLE;

struct CSVRecord {
    int index;
    std::string string;
};


TSTRINGTABLE *stringtable_load(const char *filename) {
    std::ifstream input(filename);
    if (!input) return NULL;
    CSVReader reader([&input]{return input.get();});
    auto mapping = reader.mapColumns<CSVRecord>({
        {"id", &CSVRecord::index},
        {"string", &CSVRecord::string},
    });
    if (!mapping.allMapped) return NULL;
    CSVRecord rec;
    std::unique_ptr<TSTRINGTABLE> tbl = std::make_unique<TSTRINGTABLE>();
    while (reader.readRow(mapping, rec)) {
        windows2kamenik(rec.string.data(), (int)rec.string.size(), rec.string.data());
        tbl->_strings[rec.index] = rec.string;
    }
    return tbl.release();
}

const char *stringtable_find(const TSTRINGTABLE *st, int id, const char *default_value) {
    if (st) {
        auto iter = st->_strings.find(id);
        if (iter != st->_strings.end()) {
            return iter->second.c_str();
        }
    }
    return default_value;
}
void stringtable_free(TSTRINGTABLE *st) {
    delete st;
}

