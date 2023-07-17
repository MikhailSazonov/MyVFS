#pragma once

#include "File.hpp"

namespace TestTask
{
    struct IVFS
    {
        File *Open( const char *name ) = 0; // Открыть файл в readonly режиме. Если нет такого файла или же он открыт во writeonly режиме - вернуть nullptr
        File *Create( const char *name ) = 0; // Открыть или создать файл в writeonly режиме. Если нужно, то создать все нужные поддиректории, упомянутые в пути. Вернуть nullptr, если этот файл уже открыт в readonly режиме.
        size_t Read( File *f, char *buff, size_t len ) = 0; // Прочитать данные из файла. Возвращаемое значение - сколько реально байт удалось прочитать
        size_t Write( File *f, char *buff, size_t len ) = 0; // Записать данные в файл. Возвращаемое значение - сколько реально байт удалось записать
        void Close( File *f ) = 0; // Закрыть файл	
    };
}
