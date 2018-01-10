#include "test.hpp"

namespace
{
	char const* VERT_SHADER_SOURCE("gl-430/performance-query-intel.vert");
	char const* FRAG_SHADER_SOURCE("gl-430/performance-query-intel.frag");
	char const* TEXTURE_DIFFUSE("kueken7_rgba8_srgb.dds");

	GLsizei const VertexCount(4);
	GLsizeiptr const VertexSize = VertexCount * sizeof(glf::vertex_v2fv2f);
	glf::vertex_v2fv2f const VertexData[VertexCount] =
	{
		glf::vertex_v2fv2f(glm::vec2(-1.0f,-1.0f), glm::vec2(0.0f, 1.0f)),
		glf::vertex_v2fv2f(glm::vec2( 1.0f,-1.0f), glm::vec2(1.0f, 1.0f)),
		glf::vertex_v2fv2f(glm::vec2( 1.0f, 1.0f), glm::vec2(1.0f, 0.0f)),
		glf::vertex_v2fv2f(glm::vec2(-1.0f, 1.0f), glm::vec2(0.0f, 0.0f))
	};

	GLsizei const ElementCount(6);
	GLsizeiptr const ElementSize = ElementCount * sizeof(GLushort);
	GLushort const ElementData[ElementCount] =
	{
		0, 1, 2, 
		2, 3, 0
	};

	namespace program
	{
		enum type
		{
			VERTEX,
			FRAGMENT,
			MAX
		};
	}//namespace program

	namespace buffer
	{
		enum type
		{
			VERTEX,
			ELEMENT,
			TRANSFORM,
			MAX
		};
	}//namespace buffer

	static char const* GetCounterTypeString(GLenum counterType)
	{
		switch(counterType)
		{
		case GL_PERFQUERY_COUNTER_EVENT_INTEL:
			return "EVENT";
		case GL_PERFQUERY_COUNTER_DURATION_NORM_INTEL:
			return "DURATION_NORM";
		case GL_PERFQUERY_COUNTER_DURATION_RAW_INTEL:
			return "DURATION_RAW";
		case GL_PERFQUERY_COUNTER_THROUGHPUT_INTEL:
			return "THROUGHPUT";
		case GL_PERFQUERY_COUNTER_RAW_INTEL:
			return "RAW";
		case GL_PERFQUERY_COUNTER_TIMESTAMP_INTEL:
			return "TIMESTAMP";
		default:
			return "UNKNOWN";
		}
	}

	static char const* GetCounterDataTypeString(GLenum counterDataType)
	{
		switch(counterDataType)
		{
		case GL_PERFQUERY_COUNTER_DATA_UINT32_INTEL:
			return "UINT32";
		case GL_PERFQUERY_COUNTER_DATA_UINT64_INTEL:
			return "UINT64";
		case GL_PERFQUERY_COUNTER_DATA_FLOAT_INTEL:
			return "FLOAT";
		case GL_PERFQUERY_COUNTER_DATA_DOUBLE_INTEL:
			return "DOUBLE";
		case GL_PERFQUERY_COUNTER_DATA_BOOL32_INTEL:
			return "BOOL32";
		default:
			return "UNKNOWN";
		}
	}

}//namespace

class gl_430_perf_query : public test
{
public:
	gl_430_perf_query(int argc, char* argv[]) :
		test(argc, argv, "gl-430-perf-query", test::CORE, 4, 2),
		PipelineName(0),
		VertexArrayName(0),
		TextureName(0)
	{}

private:
	GLuint PipelineName;
	GLuint VertexArrayName;
	GLuint TextureName;
	std::array<GLuint, program::MAX> ProgramName;
	std::array<GLuint, buffer::MAX> BufferName;

	bool initPerf()
	{
		GLint QueryNameMaxLength = 0;
		glGetIntegerv(GL_PERFQUERY_QUERY_NAME_LENGTH_MAX_INTEL, &QueryNameMaxLength);
		GLint CounterNameMaxLength = 0;
		glGetIntegerv(GL_PERFQUERY_COUNTER_NAME_LENGTH_MAX_INTEL, &CounterNameMaxLength);
		GLint CounterDescMaxLength = 0;
		glGetIntegerv(GL_PERFQUERY_COUNTER_DESC_LENGTH_MAX_INTEL, &CounterDescMaxLength);

		const GLuint queryNameLen = 1024;
		GLchar queryName[queryNameLen];

		const GLuint counterNameLen = 1024;
		GLchar counterName[counterNameLen];

		const GLuint counterDescLen = 4096;
		GLchar counterDesc[counterDescLen];

		GLuint queryId;
		glGetFirstPerfQueryIdINTEL(&queryId);

		while(queryId)
		{
			GLuint dataSize = 0;
			GLuint noCounters = 0;
			GLuint noInstances = 0;
			GLuint capsMask = 0;

			memset(queryName, 0, sizeof(queryName));

			glGetPerfQueryInfoINTEL(
				queryId,
				QueryNameMaxLength,
				queryName,
				&dataSize,
				&noCounters,
				&noInstances,
				&capsMask);

			printf("%d - %s (%s)/ output size: %d, counters: %d, instances: %d\n", queryId, queryName, capsMask & GL_PERFQUERY_GLOBAL_CONTEXT_INTEL ? "global" : "local", dataSize, noCounters, noInstances);

			glGetNextPerfQueryIdINTEL(queryId, &queryId);

			for(GLuint counterId = 1; counterId <= noCounters; counterId++)
			{
				GLuint counterOffset = 0;
				GLuint counterDataSize = 0;
				GLuint counterTypeEnum = 0;
				GLuint counterDataTypeEnum = 0;
				GLuint64 rawCounterMaxValue = 0;

				memset(counterName, 0, sizeof(counterName));
				memset(counterDesc, 0, sizeof(counterDesc));

				glGetPerfCounterInfoINTEL(
					queryId,
					counterId,
					CounterNameMaxLength,
					counterName,
					CounterDescMaxLength,
					counterDesc,
					&counterOffset,
					&counterDataSize,
					&counterTypeEnum,
					&counterDataTypeEnum,
					&rawCounterMaxValue);

				printf("- %d - %s output: %s - %s - %d, max counter values: %lld\n", counterId, counterName, GetCounterTypeString(counterTypeEnum), GetCounterDataTypeString(counterDataTypeEnum), dataSize, rawCounterMaxValue);
				printf("\t%s\n", counterDesc);

				continue;
			}
		}

		return true;
	}

	bool initProgram()
	{
		bool Validated(true);
	
		if(Validated)
		{
			compiler Compiler;
			GLuint VertShaderName = Compiler.create(GL_VERTEX_SHADER, getDataDirectory() + VERT_SHADER_SOURCE, "--version 420 --profile core");
			GLuint FragShaderName = Compiler.create(GL_FRAGMENT_SHADER, getDataDirectory() + FRAG_SHADER_SOURCE, "--version 420 --profile core");
			Validated = Validated && Compiler.check();

			ProgramName[program::VERTEX] = glCreateProgram();
			glProgramParameteri(ProgramName[program::VERTEX], GL_PROGRAM_SEPARABLE, GL_TRUE);
			glAttachShader(ProgramName[program::VERTEX], VertShaderName);
			glLinkProgram(ProgramName[program::VERTEX]);
			Validated = Validated && Compiler.check_program(ProgramName[program::VERTEX]);

			ProgramName[program::FRAGMENT] = glCreateProgram();
			glProgramParameteri(ProgramName[program::FRAGMENT], GL_PROGRAM_SEPARABLE, GL_TRUE);
			glAttachShader(ProgramName[program::FRAGMENT], FragShaderName);
			glLinkProgram(ProgramName[program::FRAGMENT]);
			Validated = Validated && Compiler.check_program(ProgramName[program::FRAGMENT]);
		}

		if(Validated)
		{
			glGenProgramPipelines(1, &PipelineName);
			glUseProgramStages(PipelineName, GL_VERTEX_SHADER_BIT, ProgramName[program::VERTEX]);
			glUseProgramStages(PipelineName, GL_FRAGMENT_SHADER_BIT, ProgramName[program::FRAGMENT]);
		}

		return Validated;
	}

	bool initBuffer()
	{
		bool Validated(true);

		glGenBuffers(buffer::MAX, &BufferName[0]);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BufferName[buffer::ELEMENT]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, ElementSize, ElementData, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		glBindBuffer(GL_ARRAY_BUFFER, BufferName[buffer::VERTEX]);
		glBufferData(GL_ARRAY_BUFFER, VertexSize, VertexData, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		GLint UniformBufferOffset(0);
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &UniformBufferOffset);
		GLint UniformBlockSize = glm::max(GLint(sizeof(glm::mat4)), UniformBufferOffset);

		glBindBuffer(GL_UNIFORM_BUFFER, BufferName[buffer::TRANSFORM]);
		glBufferData(GL_UNIFORM_BUFFER, UniformBlockSize, NULL, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		return Validated;
	}

	bool initTexture()
	{
		gli::texture2d Texture(gli::load_dds((getDataDirectory() + ).c_str()));
		gli::gl GL(gli::gl::PROFILE_GL33);
		gli::gl::format const Format = GL.translate(Texture.format(), Texture.swizzles());

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glGenTextures(1, &TextureName);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, TextureName);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SWIZZLE_R, Format.Swizzles[0]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SWIZZLE_G, Format.Swizzles[1]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SWIZZLE_B, Format.Swizzles[2]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_SWIZZLE_A, Format.Swizzles[3]);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(Texture.levels() - 1));
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTexStorage3D(GL_TEXTURE_2D_ARRAY, static_cast<GLint>(Texture.levels()),
			Format.Internal,
			static_cast<GLsizei>(Texture[0].extent().x), static_cast<GLsizei>(Texture[0].extent().y), static_cast<GLsizei>(1));

		for(gli::texture2d::size_type Level = 0; Level < Texture.levels(); ++Level)
		{
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, static_cast<GLint>(Level),
				0, 0, 0,
				static_cast<GLsizei>(Texture[Level].extent().x), static_cast<GLsizei>(Texture[Level].extent().y), GLsizei(1),
				Format.External, Format.Type,
				Texture[Level].data());
		}
	
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		return true;
	}

	bool initVertexArray()
	{
		bool Validated(true);

		glGenVertexArrays(1, &VertexArrayName);
		glBindVertexArray(VertexArrayName);
			glBindBuffer(GL_ARRAY_BUFFER, BufferName[buffer::VERTEX]);
			glVertexAttribPointer(semantic::attr::POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(glf::vertex_v2fv2f), BUFFER_OFFSET(0));
			glVertexAttribPointer(semantic::attr::TEXCOORD, 2, GL_FLOAT, GL_FALSE, sizeof(glf::vertex_v2fv2f), BUFFER_OFFSET(sizeof(glm::vec2)));
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			glEnableVertexAttribArray(semantic::attr::POSITION);
			glEnableVertexAttribArray(semantic::attr::TEXCOORD);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BufferName[buffer::ELEMENT]);
		glBindVertexArray(0);

		return Validated;
	}

	bool begin()
	{
		bool Validated(true);
		Validated = Validated && this->checkExtension("GL_INTEL_performance_query");

		if(Validated)
			Validated = initPerf();
		if(Validated)
			Validated = initProgram();
		if(Validated)
			Validated = initBuffer();
		if(Validated)
			Validated = initVertexArray();
		if(Validated)
			Validated = initTexture();

		return Validated;
	}

	bool end()
	{
		glDeleteProgramPipelines(1, &PipelineName);
		glDeleteProgram(ProgramName[program::FRAGMENT]);
		glDeleteProgram(ProgramName[program::VERTEX]);
		glDeleteBuffers(buffer::MAX, &BufferName[0]);
		glDeleteTextures(1, &TextureName);
		glDeleteVertexArrays(1, &VertexArrayName);

		return true;
	}

	bool render()
	{
		glm::vec2 WindowSize(this->getWindowSize());

		{
			glBindBuffer(GL_UNIFORM_BUFFER, BufferName[buffer::TRANSFORM]);
			glm::mat4* Pointer = (glm::mat4*)glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);

			glm::mat4 Projection = glm::perspectiveFov(glm::pi<float>() * 0.25f, WindowSize.x, WindowSize.y, 0.1f, 100.0f);
			glm::mat4 Model = glm::mat4(1.0f);
			*Pointer = Projection * this->view() * Model;

			// Make sure the uniform buffer is uploaded
			glUnmapBuffer(GL_UNIFORM_BUFFER);
		}

		glViewportIndexedf(0, 0, 0, WindowSize.x, WindowSize.y);
		glClearBufferfv(GL_COLOR, 0, &glm::vec4(1.0f, 0.5f, 0.0f, 1.0f)[0]);

		glBindProgramPipeline(PipelineName);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, TextureName);
		glBindVertexArray(VertexArrayName);
		glBindBufferBase(GL_UNIFORM_BUFFER, semantic::uniform::TRANSFORM0, BufferName[buffer::TRANSFORM]);

		glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, ElementCount, GL_UNSIGNED_SHORT, 0, 1, 0, 0);

		return true;
	}
};

int main(int argc, char* argv[])
{
	int Error(0);

	gl_430_perf_query Test(argc, argv);
	Error += Test();

	return Error;
}
