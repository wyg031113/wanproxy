#include <common/test.h>

#include <config/config.h>
#include <config/config_class.h>
#include <config/config_exporter.h>
#include <config/config_object.h>
#include <config/config_type_flags.h>

#define	FLAG_A	(0x00000001)
#define	FLAG_B	(0x00000002)
#define	FLAG_C	(0x00000004)

typedef ConfigTypeFlags<unsigned int> TestConfigTypeFlags;

static struct TestConfigTypeFlags::Mapping test_config_type_flags_map[] = {
	{ "A",		FLAG_A },
	{ "B",		FLAG_B },
	{ "C",		FLAG_C },
	{ NULL,		0 }
};

static TestConfigTypeFlags
	test_config_type_flags("test-flags", test_config_type_flags_map);

class TestConfigClassFlags : public ConfigClass {
public:
	TestConfigClassFlags(void)
	: ConfigClass("test-config-flags")
	{
		add_member("flags", &test_config_type_flags);
	}

	~TestConfigClassFlags()
	{ }

	bool activate(ConfigObject *co)
	{
		TestConfigTypeFlags *ct;
		ConfigValue *cv;
		unsigned int flags;

		cv = co->get("flags", &ct);
		if (cv == NULL) {
			ERROR("/test/config/flags/class") << "Could not get flags.";
			return (false);
		}

		if (!ct->get(cv, &flags)) {
			ERROR("/test/config/flags/class") << "Could not get flags1.";
			return (false);
		}

		if (flags != (FLAG_A | FLAG_C)) {
			ERROR("/test/config/flags/class") << "Field (flags) does not have expected value.";
			return (false);
		}

		INFO("/test/config/flags/class") << "Got all expected values.";

		return (true);
	}
};

class TestConfigExporter : public ConfigExporter {
	LogHandle log_;
	std::string str_;
public:
	TestConfigExporter(void)
	: log_("/test/config/exporter")
	{ }

	~TestConfigExporter()
	{ }

	void config(const Config *c)
	{
		str_ = "config:";
		c->marshall(this);
		str_ += ";";
	}

	void field(const ConfigValue *cv, const std::string& name)
	{
		str_ += " field ";
		str_ += name;
		str_ += ":";
		cv->marshall(this);
		str_ += ";";
	}

	void object(const ConfigClass *cc, const ConfigObject *co)
	{
		str_ += " object ";
		str_ += co->name();
		str_ += " class ";
		str_ += cc->name();
		str_ += ":";
		cc->marshall(this, co);
		str_ += ";";
	}

	void value(const ConfigValue *, const std::string& val)
	{
		str_ += " value: ";
		str_ += val;
		str_ += ";";
	}

	bool check(void) const
	{
		return (str_ == "config: object test class test-config-flags: field flags: value: A|C;;;;");
	}
};

int
main(void)
{
	Config config;
	TestConfigClassFlags test_config_class_flags;

	config.import(&test_config_class_flags);

	TestGroup g("/test/config/exporter1", "ConfigTypeExporter #1");
	{
		Test _(g, "Create test object.");

		if (config.create("test-config-flags", "test"))
			_.pass();
	}
	{
		Test _(g, "Set flags.");
		if (config.set("test", "flags", "A"))
			_.pass();
	}
	{
		Test _(g, "Set flags.");
		if (config.set("test", "flags", "C"))
			_.pass();
	}
	{
		Test _(g, "Activate test object.");
		if (config.activate("test"))
			_.pass();
	}

	TestConfigExporter exp;
	exp.config(&config);

	{
		Test _(g, "Check exported config.");
		if (exp.check())
			_.pass();
	}
}
