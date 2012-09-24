//**********************************
// Compiler
// 21/09/2012 - 21/09/2012
//**********************************
// Christophe Riccio
// ogl-samples@g-truc.net
//**********************************
// G-Truc Creation
// www.g-truc.net
//**********************************

namespace glf
{
	inline compiler::~compiler()
	{
		this->clear();
	}

	inline GLuint compiler::create(GLenum Type, std::string const & Filename)
	{
		return this->create(Type, std::string(), Filename);
	}

	// TODO: Add Arguments supports
	inline GLuint compiler::create(GLenum Type, std::string const & Arguments, std::string const & Filename)
	{
		assert(!Filename.empty());
	
		commandline CommandLine(Arguments);

		std::string Header;
/*
		if(CommandLine.getVersion())
			Header = format("#version %d %s\n", CommandLine.getVersion(), CommandLine.getProfile().c_str());
		Header += CommandLine.getDefines();
*/
		//if(Defines.empty())

		//std::string Line;
		//std::getline(SourceContent, Line);

		std::string ShaderSource = glf::loadFile(Filename);

		std::string PreprocessedSource = parser()(CommandLine, ShaderSource);
		char const * PreprocessedSourcePointer = PreprocessedSource.c_str();

		fprintf(stdout, "%s\n", PreprocessedSource.c_str());

		GLuint Name = glCreateShader(Type);
		glShaderSource(Name, 1, &PreprocessedSourcePointer, NULL);
		glCompileShader(Name);

		std::pair<files_map::iterator, bool> ResultFiles = this->ShaderFiles.insert(std::make_pair(Name, Filename));
		assert(ResultFiles.second);
		std::pair<names_map::iterator, bool> ResultNames = this->ShaderNames.insert(std::make_pair(Filename, Name));
		assert(ResultNames.second);
		std::pair<names_map::iterator, bool> ResultChecks = this->PendingChecks.insert(std::make_pair(Filename, Name));
		assert(ResultChecks.second);

		return Name;
	}

	inline bool compiler::destroy(GLuint const & Name)
	{
		files_map::iterator NameIterator = this->ShaderFiles.find(Name);
		if(NameIterator == this->ShaderFiles.end())
			return false; // Shader name not found
		std::string File = NameIterator->second;
		this->ShaderFiles.erase(NameIterator);

		// Remove from the pending checks list
		names_map::iterator PendingIterator = this->PendingChecks.find(File);
		if(PendingIterator != this->PendingChecks.end())
			this->PendingChecks.erase(PendingIterator);

		// Remove from the pending checks list
		names_map::iterator FileIterator = this->ShaderNames.find(File);
		assert(FileIterator != this->ShaderNames.end());
		this->ShaderNames.erase(FileIterator);

		return true;
	}

	// TODO Interaction with KHR_debug
	inline bool compiler::check()
	{
		bool Success(true);

		for
		(
			names_map::iterator ShaderIterator = PendingChecks.begin();
			ShaderIterator != PendingChecks.end();
			++ShaderIterator
		)
		{
			GLuint ShaderName = ShaderIterator->second;
			GLint Result = GL_FALSE;
			glGetShaderiv(ShaderName, GL_COMPILE_STATUS, &Result);

			if(Result == GL_TRUE)
			{
				int InfoLogLength;
				glGetShaderiv(ShaderName, GL_INFO_LOG_LENGTH, &InfoLogLength);
				if(InfoLogLength > 0)
				{
					std::vector<char> Buffer(InfoLogLength);
					glGetShaderInfoLog(ShaderName, InfoLogLength, NULL, &Buffer[0]);
					fprintf(stdout, "%s\n", &Buffer[0]);
				}
			}

			Success = Success && Result == GL_TRUE;
		}
	
		return Success; 
	}

	inline void compiler::clear()
	{
		for
		(
			names_map::iterator ShaderNameIterator = this->ShaderNames.begin(); 
			ShaderNameIterator != this->ShaderNames.end(); 
			++ShaderNameIterator
		)
			glDeleteShader(ShaderNameIterator->second);
		this->ShaderNames.clear();
		this->ShaderFiles.clear();
		this->PendingChecks.clear();
	}

	inline bool compiler::loadBinary
	(
		std::string const & Filename,
		GLenum & Format,
		std::vector<glm::byte> & Data,
		GLint & Size
	)
	{
		FILE* File = fopen(Filename.c_str(), "rb");

		if(File)
		{
			fread(&Format, sizeof(GLenum), 1, File);
			fread(&Size, sizeof(Size), 1, File);
			Data.resize(Size);
			fread(&Data[0], Size, 1, File);
			fclose(File);
			return true;
		}
		return false;
	}

	inline std::string compiler::loadFile
	(
		std::string const & Filename
	)
	{
		std::ifstream stream(Filename.c_str(), std::ios::in);

		if(!stream.is_open())
			return "";

		std::string Line = "";
		std::string Text = "";

		while(getline(stream, Line))
			Text += "\n" + Line;

		stream.close();

		return Text;
	}
}//namespace glf
