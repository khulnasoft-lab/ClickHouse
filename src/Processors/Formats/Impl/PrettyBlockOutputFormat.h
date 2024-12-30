#pragma once

#include <Core/Block.h>
#include <Processors/Formats/IOutputFormat.h>
#include <Formats/FormatSettings.h>
#include <Formats/FormatFactory.h>

namespace DB
{

class WriteBuffer;
class Context;


/** Prints the result in the form of beautiful tables.
  */
class PrettyBlockOutputFormat : public IOutputFormat
{
public:
    /// no_escapes - do not use ANSI escape sequences - to display in the browser, not in the console.
    PrettyBlockOutputFormat(WriteBuffer & out_, const Block & header_, const FormatSettings & format_settings_, bool mono_block_, bool color_);
    ~PrettyBlockOutputFormat() override;
    String getName() const override { return "PrettyBlockOutputFormat"; }
protected:
    void consume(Chunk) override;
    void consumeTotals(Chunk) override;
    void consumeExtremes(Chunk) override;

    size_t total_rows = 0;
    size_t displayed_rows = 0;
    size_t row_number_width = 7; // "10000. "

    const FormatSettings format_settings;
    Serializations serializations;

    using Widths = PODArray<size_t>;
    using WidthsPerColumn = std::vector<Widths>;

    void write(Chunk chunk, PortKind port_kind);
    virtual void writeChunk(const Chunk & chunk, PortKind port_kind);
    void writeMonoChunkIfNeeded();
    void writeSuffix() override;
    virtual void writeSuffixImpl();

    void onRowsReadBeforeUpdate() override { total_rows = getRowsReadBefore(); }

    void calculateWidths(
        const Block & header, const Chunk & chunk,
        WidthsPerColumn & widths, Widths & max_padded_widths, Widths & name_widths, Strings & names);

    void writeValueWithPadding(
        const IColumn & column, const ISerialization & serialization, size_t row_num,
        bool split_by_lines, std::optional<String> & serialized_value, size_t & start_from_offset,
        size_t value_width, size_t pad_to_width, size_t cut_to_width, bool align_right, bool is_number);

    void resetFormatterImpl() override
    {
        total_rows = 0;
        displayed_rows = 0;
    }

    static bool cutInTheMiddle(size_t row_num, size_t num_rows, size_t max_rows);

    bool color;
    bool readable_number_tip = false;

private:
    bool mono_block;
    /// For mono_block == true only
    Chunk mono_chunk;
    /// Implements squashing of chunks by time
    std::condition_variable mono_chunk_condvar;
    std::optional<ThreadFromGlobalPool> thread;
    bool finish = false;
    void writingThread();
    void stopThread();
};

template <typename OutputFormat>
void registerPrettyFormatWithNoEscapesAndMonoBlock(FormatFactory & factory, const String & base_name)
{
    auto creator = [&](FormatFactory & fact, const String & name, bool no_escapes, bool mono_block)
    {
        fact.registerOutputFormat(name, [no_escapes, mono_block](
            WriteBuffer & buf,
            const Block & sample,
            const FormatSettings & format_settings)
        {
            bool color = !no_escapes
                    && (format_settings.pretty.color == 1 || (format_settings.pretty.color == 2 && format_settings.is_writing_to_terminal));
            return std::make_shared<OutputFormat>(buf, sample, format_settings, mono_block, color);
        });
    };
    creator(factory, base_name, false, false);
    creator(factory, base_name + "NoEscapes", true, false);
    creator(factory, base_name + "MonoBlock", false, true);
    creator(factory, base_name + "NoEscapesMonoBlock", true, true);
}

}
