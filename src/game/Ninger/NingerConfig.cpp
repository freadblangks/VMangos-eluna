#include "NingerConfig.h"
#include "Policies/SingletonImp.h"

#ifndef ROBOT_CONFIG_FILE_NAME
# define ROBOT_CONFIG_FILE_NAME  "robot.conf"
#endif

INSTANTIATE_SINGLETON_1(NingerConfig);

// Defined here as it must not be exposed to end-users.
bool NingerConfig::GetValueHelper(char const* name, ACE_TString& result)
{
	GuardType guard(m_configLock);

	if (!mConf)
		return false;

	ACE_TString section_name;
	ACE_Configuration_Section_Key section_key;
	ACE_Configuration_Section_Key const& root_key = mConf->root_section();

	int i = 0;
	while (mConf->enumerate_sections(root_key, i, section_name) == 0)
	{
		mConf->open_section(root_key, section_name.c_str(), 0, section_key);
		if (mConf->get_string_value(section_key, name, result) == 0)
			return true;
		++i;
	}

	return false;
}

NingerConfig::NingerConfig()
	: mConf(nullptr)
{
}

NingerConfig::~NingerConfig()
{
	delete mConf;
}

bool NingerConfig::SetSource(char const* file)
{
	mFilename = file;

	return Reload();
}

bool NingerConfig::Reload()
{
	delete mConf;
	mConf = new ACE_Configuration_Heap;

	if (mConf->open() != -1)
	{
		ACE_Ini_ImpExp config_importer(*mConf);
		if (config_importer.import_config(mFilename.c_str()) != -1)
			return true;
	}

	delete mConf;
	mConf = nullptr;
	return false;
}

std::string NingerConfig::GetStringDefault(char const* name, char const* def)
{
	ACE_TString val;
	return GetValueHelper(name, val) ? val.c_str() : def;
}

bool NingerConfig::GetBoolDefault(char const* name, bool def)
{
	ACE_TString val;
	if (!GetValueHelper(name, val))
		return def;

	char const* str = val.c_str();
	return strcmp(str, "true") == 0 || strcmp(str, "TRUE") == 0 ||
		strcmp(str, "yes") == 0 || strcmp(str, "YES") == 0 ||
		strcmp(str, "1") == 0;
}


int32 NingerConfig::GetIntDefault(char const* name, int32 def)
{
	ACE_TString val;
	return GetValueHelper(name, val) ? atoi(val.c_str()) : def;
}


float NingerConfig::GetFloatDefault(char const* name, float def)
{
	ACE_TString val;
	return GetValueHelper(name, val) ? (float)atof(val.c_str()) : def;
}

bool NingerConfig::StartNingerSystem()
{
	SetSource(ROBOT_CONFIG_FILE_NAME);
	Reload();

	Enable = GetIntDefault("Enable", 0);
	AccountNamePrefix = GetStringDefault("AccountNamePrefix", "ROBOT");
	DPSDelay = GetIntDefault("DPSDelay", 2000);	
	if (Enable == 0)
	{
		sLog.outBasic("Robot system is disabled.");
		return false;
	}
	sLog.outBasic("Robot system started.");
	return true;
}

std::vector<std::string> NingerConfig::SplitString(std::string srcStr, std::string delimStr, bool repeatedCharIgnored)
{
	std::vector<std::string> resultStringVector;
	std::replace_if(srcStr.begin(), srcStr.end(), [&](const char& c) {if (delimStr.find(c) != std::string::npos) { return true; } else { return false; }}/*pred*/, delimStr.at(0));
	size_t pos = srcStr.find(delimStr.at(0));
	std::string addedString = "";
	while (pos != std::string::npos) {
		addedString = srcStr.substr(0, pos);
		if (!addedString.empty() || !repeatedCharIgnored) {
			resultStringVector.push_back(addedString);
		}
		srcStr.erase(srcStr.begin(), srcStr.begin() + pos + 1);
		pos = srcStr.find(delimStr.at(0));
	}
	addedString = srcStr;
	if (!addedString.empty() || !repeatedCharIgnored) {
		resultStringVector.push_back(addedString);
	}
	return resultStringVector;
}

std::string NingerConfig::TrimString(std::string srcStr)
{
	std::string result = srcStr;
	if (!result.empty())
	{
		result.erase(0, result.find_first_not_of(" "));
		result.erase(result.find_last_not_of(" ") + 1);
	}

	return result;
}