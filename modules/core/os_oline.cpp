/* OperServ core functions
 *
 * (C) 2003-2010 Anope Team
 * Contact us at team@anope.org
 *
 * Please read COPYING and README for further details.
 *
 * Based on the original code of Epona by Lara.
 * Based on the original code of Services by Andy Church.
 */

/*************************************************************************/

#include "module.h"

class CommandOSOLine : public Command
{
 public:
	CommandOSOLine() : Command("OLINE", 2, 2, "operserv/oline")
	{
	}

	CommandReturn Execute(User *u, const std::vector<Anope::string> &params)
	{
		Anope::string nick = params[0];
		Anope::string flag = params[1];
		User *u2 = NULL;

		/* let's check whether the user is online */
		if (!(u2 = finduser(nick)))
			notice_lang(Config.s_OperServ, u, NICK_X_NOT_IN_USE, nick.c_str());
		else if (u2 && flag[0] == '+')
		{
			ircdproto->SendSVSO(Config.s_OperServ, nick, flag);
			u2->SetMode(OperServ, UMODE_OPER);
			notice_lang(Config.s_OperServ, u2, OPER_OLINE_IRCOP);
			notice_lang(Config.s_OperServ, u, OPER_OLINE_SUCCESS, flag.c_str(), nick.c_str());
			ircdproto->SendGlobops(OperServ, "\2%s\2 used OLINE for %s", u->nick.c_str(), nick.c_str());
		}
		else if (u2 && flag[0] == '-')
		{
			ircdproto->SendSVSO(Config.s_OperServ, nick, flag);
			notice_lang(Config.s_OperServ, u, OPER_OLINE_SUCCESS, flag.c_str(), nick.c_str());
			ircdproto->SendGlobops(OperServ, "\2%s\2 used OLINE for %s", u->nick.c_str(), nick.c_str());
		}
		else
			this->OnSyntaxError(u, "");
		return MOD_CONT;
	}

	bool OnHelp(User *u, const Anope::string &subcommand)
	{
		notice_help(Config.s_OperServ, u, OPER_HELP_OLINE);
		return true;
	}

	void OnSyntaxError(User *u, const Anope::string &subcommand)
	{
		syntax_error(Config.s_OperServ, u, "OLINE", OPER_OLINE_SYNTAX);
	}

	void OnServHelp(User *u)
	{
		notice_lang(Config.s_OperServ, u, OPER_HELP_CMD_OLINE);
	}
};

class OSOLine : public Module
{
 public:
	OSOLine(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator)
	{
		if (!ircd->omode)
			throw ModuleException("Your IRCd does not support OMODE.");

		this->SetAuthor("Anope");
		this->SetType(CORE);

		this->AddCommand(OperServ, new CommandOSOLine());
	}
};

MODULE_INIT(OSOLine)