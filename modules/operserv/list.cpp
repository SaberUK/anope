/*
 * Anope IRC Services
 *
 * Copyright (C) 2003-2016 Anope Team <team@anope.org>
 *
 * This file is part of Anope. Anope is free software; you can
 * redistribute it and/or modify it under the terms of the GNU
 * General Public License as published by the Free Software
 * Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see see <http://www.gnu.org/licenses/>.
 */

#include "module.h"

class CommandOSChanList : public Command
{
 public:
	CommandOSChanList(Module *creator) : Command(creator, "operserv/chanlist", 0, 2)
	{
		this->SetDesc(_("Lists all channel records"));
		this->SetSyntax(_("[{\037pattern\037 | \037nick\037} [\037SECRET\037]]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		const Anope::string &pattern = !params.empty() ? params[0] : "";
		const Anope::string &opt = params.size() > 1 ? params[1] : "";
		std::set<Anope::string> modes;
		User *u2;

		if (!pattern.empty())
			logger.Admin(source, _("{source} used {command} for {0}"), pattern);
		else
			logger.Admin(source, _("{source} used {command}"));

		if (!opt.empty() && opt.equals_ci("SECRET"))
		{
			modes.insert("SECRET");
			modes.insert("PRIVATE");
		}

		ListFormatter list(source.GetAccount());
		list.AddColumn(_("Name")).AddColumn(_("Users")).AddColumn(_("Modes")).AddColumn(_("Topic"));

		if (!pattern.empty() && (u2 = User::Find(pattern, true)))
		{
			source.Reply(_("Channel list for \002{0}\002:"), u2->nick);

			for (User::ChanUserList::iterator uit = u2->chans.begin(), uit_end = u2->chans.end(); uit != uit_end; ++uit)
			{
				ChanUserContainer *cc = uit->second;

				if (!modes.empty())
					for (std::set<Anope::string>::iterator it = modes.begin(), it_end = modes.end(); it != it_end; ++it)
						if (!cc->chan->HasMode(*it))
							continue;

				ListFormatter::ListEntry entry;
				entry["Name"] = cc->chan->name;
				entry["Users"] = stringify(cc->chan->users.size());
				entry["Modes"] = cc->chan->GetModes(true, true);
				entry["Topic"] = cc->chan->topic;
				list.AddEntry(entry);
			}
		}
		else
		{
			source.Reply(_("Channel list:"));

			for (channel_map::const_iterator cit = ChannelList.begin(), cit_end = ChannelList.end(); cit != cit_end; ++cit)
			{
				Channel *c = cit->second;

				if (!pattern.empty() && !Anope::Match(c->name, pattern, false, true))
					continue;
				if (!modes.empty())
					for (std::set<Anope::string>::iterator it = modes.begin(), it_end = modes.end(); it != it_end; ++it)
						if (!c->HasMode(*it))
							continue;

				ListFormatter::ListEntry entry;
				entry["Name"] = c->name;
				entry["Users"] = stringify(c->users.size());
				entry["Modes"] = c->GetModes(true, true);
				entry["Topic"] = c->topic;
				list.AddEntry(entry);
			}
		}

		std::vector<Anope::string> replies;
		list.Process(replies);

		for (unsigned i = 0; i < replies.size(); ++i)
			source.Reply(replies[i]);

		source.Reply(_("End of channel list."));
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		source.Reply(_("Lists all channels currently in use on the IRC network, whether they are registered or not.\n"
		               "\n"
		               "If \002pattern\002 is given, lists only channels that match it. If a nicknam is given, lists only the channels the user using it is on."
		               " If SECRET is specified, lists only channels matching \002pattern\002 that have the +s or +p mode."));

		const Anope::string &regexengine = Config->GetBlock("options")->Get<Anope::string>("regexengine");
		if (!regexengine.empty())
		{
			source.Reply(" ");
			source.Reply(_("Regex matches are also supported using the {0} engine. Enclose your pattern in // if this is desired."), regexengine);
		}

		return true;
	}
};

class CommandOSUserList : public Command
{
 public:
	CommandOSUserList(Module *creator) : Command(creator, "operserv/userlist", 0, 2)
	{
		this->SetDesc(_("Lists all user records"));
		this->SetSyntax(_("[{\037pattern\037 | \037channel\037} [\037INVISIBLE\037]]"));
	}

	void Execute(CommandSource &source, const std::vector<Anope::string> &params) override
	{
		const Anope::string &pattern = !params.empty() ? params[0] : "";
		const Anope::string &opt = params.size() > 1 ? params[1] : "";
		Channel *c;
		std::set<Anope::string> modes;

		if (!pattern.empty())
			logger.Admin(source, _("{source} used {command} for {0}"), pattern);
		else
			logger.Admin(source, _("{source} used {command}"));

		if (!opt.empty() && opt.equals_ci("INVISIBLE"))
			modes.insert("INVIS");

		ListFormatter list(source.GetAccount());
		list.AddColumn(_("Name")).AddColumn(_("Mask"));

		if (!pattern.empty() && (c = Channel::Find(pattern)))
		{
			source.Reply(_("User list for \002{0}\002:"), pattern);

			for (Channel::ChanUserList::iterator cuit = c->users.begin(), cuit_end = c->users.end(); cuit != cuit_end; ++cuit)
			{
				ChanUserContainer *uc = cuit->second;

				if (!modes.empty())
					for (std::set<Anope::string>::iterator it = modes.begin(), it_end = modes.end(); it != it_end; ++it)
						if (!uc->user->HasMode(*it))
							continue;

				ListFormatter::ListEntry entry;
				entry["Name"] = uc->user->nick;
				entry["Mask"] = uc->user->GetIdent() + "@" + uc->user->GetDisplayedHost();
				list.AddEntry(entry);
			}
		}
		else
		{
			/* Historically this has been ordered, so... */
			Anope::map<User *> ordered_map;
			for (user_map::const_iterator it = UserListByNick.begin(); it != UserListByNick.end(); ++it)
				ordered_map[it->first] = it->second;

			source.Reply(_("Users list:"));

			for (Anope::map<User *>::const_iterator it = ordered_map.begin(); it != ordered_map.end(); ++it)
			{
				User *u2 = it->second;

				if (!pattern.empty())
				{
					Anope::string mask = u2->nick + "!" + u2->GetIdent() + "@" + u2->GetDisplayedHost(),
						mask2 = u2->nick + "!" + u2->GetIdent() + "@" + u2->host,
						mask3 = u2->nick + "!" + u2->GetIdent() + "@" + u2->ip.addr();

					if (!Anope::Match(mask, pattern, false, true)
							&& !Anope::Match(mask2, pattern, false, true)
							&& !Anope::Match(mask3, pattern, false, true))
						continue;
					if (!modes.empty())
						for (std::set<Anope::string>::iterator mit = modes.begin(), mit_end = modes.end(); mit != mit_end; ++mit)
							if (!u2->HasMode(*mit))
								continue;
				}

				ListFormatter::ListEntry entry;
				entry["Name"] = u2->nick;
				entry["Mask"] = u2->GetIdent() + "@" + u2->GetDisplayedHost();
				list.AddEntry(entry);
			}
		}

		std::vector<Anope::string> replies;
		list.Process(replies);

		for (unsigned i = 0; i < replies.size(); ++i)
			source.Reply(replies[i]);

		source.Reply(_("End of users list."));
		return;
	}

	bool OnHelp(CommandSource &source, const Anope::string &subcommand) override
	{
		this->SendSyntax(source);
		source.Reply(" ");
		source.Reply(_("Lists all users currently online on the IRC network, whether their nickname is registered or not.\n"
		               "\n"
		               "If \002pattern\002 is given, lists only users that match it (it must be in the format nick!user@host)."
		               " If \002channel\002 is given, lists only users that are on the given channel. If INVISIBLE is specified, only users with the +i flag will be listed."));

		const Anope::string &regexengine = Config->GetBlock("options")->Get<Anope::string>("regexengine");
		if (!regexengine.empty())
		{
			source.Reply(" ");
			source.Reply(_("Regex matches are also supported using the %s engine.\n"
					"Enclose your pattern in // if this is desired."), regexengine.c_str());
		}

		return true;
	}
};

class OSList : public Module
{
	CommandOSChanList commandoschanlist;
	CommandOSUserList commandosuserlist;

 public:
	OSList(const Anope::string &modname, const Anope::string &creator) : Module(modname, creator, VENDOR)
		, commandoschanlist(this)
		, commandosuserlist(this)
	{

	}
};

MODULE_INIT(OSList)