#include <ctime>
#include <fstream>
#include <iostream>
#include "public/fpdfview.h"
#include "core/fxcrt/fx_stream.h"
#include "fpdfsdk/cpdfsdk_helpers.h"
#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_parser.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_document.h"
#include "core/fpdfapi/page/cpdf_docpagedata.h"
#include "core/fpdfapi/render/cpdf_docrenderdata.h"

using namespace std;
using namespace pdfium;

/**
 * Skip some filters
 *
 * @param name - filter name
 * @return stream decode level
 */
bool getLevel(const string &name) {
    return !((name == "DCTDecode") || (name == "JPXDecode") || (name == "CCITTFaxDecode"));
}

/**
 * Print dictionary
 *
 * @param dictionary - dictionary object
 * @param old_level - old stream decode level
 * @return stream decode level
 */
bool printDictionary(CPDF_Dictionary *dictionary, bool old_level) {
    cout << "<< ";
    bool level = old_level;
    for (fxcrt::ByteString &key : dictionary->GetKeys()) {
        cout << "/" << key.c_str();
        CPDF_Object *second = dictionary->GetDirectObjectFor(key);
        if (second->IsName()) {
            const string &name = second->GetString().c_str();
            if (level)level = getLevel(name);
            cout << " /" << name << " ";
        }
        else if (second->IsNumber())
            cout << " " << second->GetNumber() << " ";
        else if (second->IsArray()) {
            CPDF_Array *array = second->AsArray();
            for (unsigned long i=0; i < array->size(); i++) {
                cout << " ";
                CPDF_Object *item = array->GetObjectAt(i);
                if (item->IsDictionary()) {
                    bool new_level = printDictionary(item->AsDictionary(), level);
                    if (level)level = new_level;
                }
                else if (item->IsName()) {
                    const string &name = item->GetString().c_str();
                    if (level)level = getLevel(name);
                    cout << item->GetString().c_str();
                }
                cout << " ";
            }
        }
    }
    cout << " >>";
    return level;
}

/**
 * PDFium library example
 *
 * Used Pdfium library (http://podofo.sourceforge.net/) licensed under the Apache License, Version 2.0
 * Documentation: http://podofo.sourceforge.net/doc/html/index.html
 * Dependencies: freetype2, pthread
 * g++ -Iinclude -I/usr/include/freetype2 main.cpp libpdfium.a -lpthread
 */
int main(int argc, char **argv) {
    // Start
    clock_t begin = clock();
    // Check parameters
    if(argc - 2) {
        cout << "input file path required" << endl;
        return 1;
    }
    cout << "Parsing file '" << argv[1] << "':" << endl;
    FPDF_InitLibrary();
    auto pDocument = pdfium::MakeUnique<CPDF_Document>(pdfium::MakeUnique<CPDF_DocRenderData>(),
                                                       pdfium::MakeUnique<CPDF_DocPageData>());
    auto pFileAccess = IFX_SeekableReadStream::CreateFromFilename(argv[1]);
    CPDF_Parser::Error error = pDocument->LoadDoc(pFileAccess, nullptr);
    if (error != CPDF_Parser::SUCCESS) {
        ProcessParseError(error);
        std::cout << "Parse error" << std::endl;
        return 1;
    }
    CPDF_Document *doc = pDocument.release();
    CPDF_Parser *parser = doc->GetParser();
    for (unsigned int i = 1; i <= parser->GetLastObjNum(); ++i) {
        if (parser->IsValidObjectNumber(i)) {
            auto obj = parser->ParseIndirectObject(i);
            if (obj.Get() && obj->IsStream()) {
                bool stream_decode = true;
                std::cout << "    Object " << i << " has stream ";
                stream_decode = printDictionary(obj->GetDict(), stream_decode);
                CPDF_Stream *object_stream = obj->AsStream();
                // Output file name
                ostringstream file_name;
                file_name << "pdf_";
                file_name.width(4);
                file_name.fill ('0');
                file_name << i << "_0.dat";
                auto data_buffer = pdfium::MakeRetain<CPDF_StreamAcc>(object_stream);
                if (stream_decode)
                    data_buffer->LoadAllDataFiltered();
                else {
                    data_buffer->LoadAllDataRaw();
                    cout << " (filter omitted)";
                }
                unsigned char *data = data_buffer->GetData();
                ofstream stream_file(file_name.str());
                for (unsigned int j=0; j < data_buffer->GetSize(); stream_file << *data++, j++);
                cout << endl;
            }
        }
    }
    delete doc;
    FPDF_DestroyLibrary();
    // Execution time output
    clock_t end = clock();
    double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    cout << endl << "Execution time: " << time_spent << " sec." << endl;
    return 0;
}
