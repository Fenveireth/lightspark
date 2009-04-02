/**************************************************************************
    Lighspark, a free flash player implementation

    Copyright (C) 2009  Alessandro Pignotti (a.pignotti@sssup.it)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include <streambuf>
#include <semaphore.h>

class sync_stream: public std::streambuf
{
public:
	sync_stream();
	std::streamsize xsgetn ( char * s, std::streamsize n );
	std::streamsize xsputn ( const char * s, std::streamsize n );
	std::streampos seekpos ( std::streampos sp, std::ios_base::openmode which/* = std::ios_base::in | std::ios_base::out*/ );
	std::streampos seekoff ( std::streamoff off, std::ios_base::seekdir way, std::ios_base::openmode which);/* = 
			std::ios_base::in | std::ios_base::out );*/
	std::streamsize showmanyc( );
private:
	char* buffer;
	int head;
	int tail;
	sem_t mutex;
	sem_t empty;
	sem_t full;
	sem_t ready;
	int wait;

	const int buf_size;
};
