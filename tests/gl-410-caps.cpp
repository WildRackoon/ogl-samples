#include "test.hpp"

class gl_410_caps : public test
{
public:
	gl_410_caps(int argc, char* argv[]) :
		test(argc, argv, "gl-410-caps", test::CORE, 4, 1)
	{}

private:
	bool checkCaps()
	{
		caps Caps(caps::CORE);

		return true;
	}

	bool begin()
	{
		return checkCaps();
	}

	bool end()
	{
		return true;
	}

	bool render()
	{
		glm::uvec2 WindowSize = this->getWindowSize();

		glViewport(0, 0, WindowSize.x, WindowSize.y);
		glClearBufferfv(GL_COLOR, 0, &glm::vec4(1.0f, 0.5f, 0.0f, 1.0f)[0]);

		return true;
	}
};

int main(int argc, char* argv[])
{
	int Error(0);

	gl_410_caps Test(argc, argv);
	Error += Test();

	return Error;
}
