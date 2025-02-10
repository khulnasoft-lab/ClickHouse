#include <atomic>
#include <Columns/IColumn.h>
#include <Storages/ColumnsDescription.h>
#include <Common/CurrentMetrics.h>
#include <DataTypes/DataTypeString.h>
#include <DataTypes/DataTypesNumber.h>
#include <DataTypes/DataTypeMap.h>
#include <Storages/System/StorageSystemMetrics.h>
#include <Common/Histogram.h>


namespace DB
{

ColumnsDescription StorageSystemMetrics::getColumnsDescription()
{
    auto description = ColumnsDescription
    {
        {"metric", std::make_shared<DataTypeString>(), "Metric name."},
        {"value", std::make_shared<DataTypeInt64>(), "Metric value."},
        {"description", std::make_shared<DataTypeString>(), "Metric description."},
        {"labels", std::make_shared<DataTypeMap>(std::make_shared<DataTypeString>(), std::make_shared<DataTypeString>()), "Metric labels."},
    };

    description.setAliases({
        {"name", std::make_shared<DataTypeString>(), "metric"}
    });

    return description;
}

void StorageSystemMetrics::fillData(MutableColumns & res_columns, ContextPtr, const ActionsDAG::Node *, std::vector<UInt8>) const
{
    for (size_t i = 0, end = CurrentMetrics::end(); i < end; ++i)
    {
        Int64 value = CurrentMetrics::values[i].load(std::memory_order_relaxed);

        res_columns[0]->insert(CurrentMetrics::getName(CurrentMetrics::Metric(i)));
        res_columns[1]->insert(value);
        res_columns[2]->insert(CurrentMetrics::getDocumentation(CurrentMetrics::Metric(i)));
        res_columns[3]->insertDefault();
    }

    const auto & factory = Histogram::Factory::instance();
    for (const auto & record : factory.getRecords())
    {
        const auto & family = record->family;
        const auto & buckets = family.getBuckets();
        const auto & labels = family.getLabels();

        for (const auto & [label_values, metric] : family.getMetrics())
        {
            Map labels_map;
            for (size_t i = 0; i < label_values.size(); ++i)
            {
                labels_map.push_back(Tuple{labels[i], label_values[i]});
            }

            // _bucket metrics
            UInt64 partial_sum = 0;
            for (size_t counter_idx = 0; counter_idx < buckets.size() + 1; ++counter_idx)
            {
                partial_sum += metric->getCounter(counter_idx);

                const String le = counter_idx < buckets.size() ? std::to_string(buckets[counter_idx]) : "+Inf";
                labels_map.push_back(Tuple{"le", le});

                res_columns[0]->insert(record->name + "_bucket");
                res_columns[1]->insert(partial_sum);
                res_columns[2]->insert(record->documentation);
                res_columns[3]->insert(labels_map);

                labels_map.pop_back();
            }

            // _count metric
            res_columns[0]->insert(record->name + "_count");
            res_columns[1]->insert(partial_sum);
            res_columns[2]->insert(record->documentation);
            res_columns[3]->insert(labels_map);

            // _sum metric
            res_columns[0]->insert(record->name + "_sum");
            res_columns[1]->insert(metric->getSum());
            res_columns[2]->insert(record->documentation);
            res_columns[3]->insert(labels_map);
        }
    }
}

}
