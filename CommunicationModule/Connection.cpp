#include "Connection.h"

namespace TpQt4Communication
{
	Connection::Connection(const Credentials &credentials) : user_(NULL), state_(STATE_CONNECTING), id_(""), protocol_( credentials.GetProtocol() ), server_( credentials.GetServer() ) , tp_connection_(NULL)
	{
	}

	Connection::~Connection()
	{
		if (!tp_connection_.isNull())
			tp_connection_->requestDisconnect();
	}

	void Connection::Close()
	{
		
		if (state_ != STATE_OPEN )
		{
			std::string message = "Connection is not open.";
			LogError(message);
			return;
		}

		LogError("Try to disconnect IM server connection.");
		Tp::PendingOperation* p = tp_connection_->requestDisconnect();
	}
	
	Connection::State Connection::GetState()
	{
		return state_;
	}

	std::string Connection::GetServer()
	{
		return server_;
	}

	User* Connection::GetUser()
	{
		return user_;
	}

	std::string Connection::GetProtocol()
	{
		return protocol_;
	}

	ChatSessionPtr Connection::CreateChatSession(Contact* contact)
	{
		if (contact == NULL || contact->tp_contact_.isNull())
		{
			QString message = "Try to open text chat session with unvalid contacts.";
			LogError(message.toStdString());
			throw message;
		}
		
		Tp::ReferencedHandles rhs =	contact->tp_contact_->handle();
		uint handle = rhs[0]; // we assume the the first 

		QVariantMap params;
		params.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType"), QLatin1String(TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT));
		params.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandleType"), Tp::HandleTypeContact);
		params.insert(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".TargetHandle"), handle);

		Tp::PendingChannel* p = tp_connection_->ensureChannel(params);
		Tp::PendingOperation *operation = (Tp::PendingOperation*)p;

		ChatSession* session = new ChatSession();
		
		QObject::connect(operation,
			SIGNAL( finished(Tp::PendingOperation*) ),
			(QObject*)session,
			SLOT( OnTextChannelCreated(Tp::PendingOperation*) ) );

		return ChatSessionPtr(session);
	}

	void Connection::OnConnectionCreated(Tp::PendingOperation *op)
	{
	    if (op->isError())
		{
			std::string message = "Cannot connect to IM server: ";
			message.append( op->errorMessage().toStdString() );
			LogError(message);
			state_ = STATE_ERROR;
			QString m;
			m.append(message.c_str());
			emit Error(m);
			return;
		}
		
		Tp::PendingConnection *c = qobject_cast<Tp::PendingConnection *>(op);
		tp_connection_ = c->connection();

		std::string message = "Connected to IM server. Start login process";
		LogInfo(message);

		QObject::connect(tp_connection_->requestConnect(),
					     SIGNAL(finished(Tp::PendingOperation *)),
						 SLOT(OnConnectionConnected(Tp::PendingOperation *)));

		QObject::connect(tp_connection_.data(),
			             SIGNAL(invalidated(Tp::DBusProxy *, const QString &, const QString &)),
						 SLOT(OnConnectionInvalidated(Tp::DBusProxy *, const QString &, const QString &)));
	}

	void Connection::OnConnectionConnected(Tp::PendingOperation *op)
	{
		if (op->isError())
		{
			QString reason = "Connection cannot become connected: ";
			reason.append(op->errorMessage());
			emit Error(reason);
			LogError(reason.toStdString());
			state_ = STATE_ERROR;
			return;
		}
		std::string message = "Connection established successfully to IM server.";
		LogInfo(message);

		if ( tp_connection_.isNull() )
			LogDebug("tp_connection_ == NULL");

		Tp::Features features;
		features.insert(Tp::Connection::FeatureSimplePresence);
		features.insert(Tp::Connection::FeatureRoster);
		features.insert(Tp::Connection::FeatureSelfContact);
		features.insert(Tp::Connection::FeatureCore);
		QObject::connect(tp_connection_->becomeReady(features),
		                 SIGNAL(finished(Tp::PendingOperation *)),
						 SLOT(OnConnectionReady(Tp::PendingOperation *)));

		if(tp_connection_->interfaces().contains(TELEPATHY_INTERFACE_CONNECTION_INTERFACE_REQUESTS))
		{
			QObject::connect(tp_connection_->requestsInterface(),
                SIGNAL(NewChannels(const Tp::ChannelDetailsList&)),
                SLOT(OnNewChannels(const Tp::ChannelDetailsList&)));
		}
	}

	void Connection::OnConnectionReady(Tp::PendingOperation *op)
	{
		LogInfo(" Connection::OnConnectionReady");
	    if (op->isError())
		{
			QString message = "Connection cannot become ready: ";
			message.append(op->errorMessage());
			emit Error(message);
			LogError(message.toStdString());
			state_ = STATE_ERROR;
	        return;
		}

		LogInfo("Create user.");
		user_ = new User(tp_connection_);

		// Build Friendlist
		Tp::Contacts all_known_contacts = tp_connection_->contactManager()->allKnownContacts();
		ContactVector new_contacts;
		foreach (const Tp::ContactPtr &contact, tp_connection_->contactManager()->allKnownContacts())
		{
			switch (contact->subscriptionState())
			{
			case Tp::Contact::PresenceStateNo:
				LogInfo("subscriptionState = PresenceStateNo");
				// User have already make a decicion to not accpet this contact to the friend list..
				break;

			case Tp::Contact::PresenceStateYes:
				{
					LogInfo("subscriptionState = PresenceStateYes");
					// A friend list item
					Contact* c = new Contact(contact);
					new_contacts.push_back(c);
				}
				break;

			case Tp::Contact::PresenceStateAsk:
				{
					LogInfo("subscriptionState = PresenceStateAsk");
					// User have not yet made the decision to accept or reject presence subscription 
					// So we create a FriendRequest obeject
					FriendRequest* request = new FriendRequest(contact);
					received_friend_requests_.push_back(request);
					emit ReceivedFriendRequest(request);
				}
				break;
			}
			switch (contact->publishState())
			{
			case Tp::Contact::PresenceStateNo:
				LogInfo("publishState = PresenceStateNo");
				break;

			case Tp::Contact::PresenceStateYes:
				LogInfo("publishState = PresenceStateYes");
				break;

			case Tp::Contact::PresenceStateAsk:
				LogInfo("publishState = PresenceStateAsk");
				break;
			}
        }
		
		QObject::connect(tp_connection_->contactManager(),
            SIGNAL(presencePublicationRequested(const Tp::Contacts &)),
            SLOT(OnPresencePublicationRequested(const Tp::Contacts &)));


		if (all_known_contacts.count() > 0)
		{
			user_->AddContacts(new_contacts);
		}

		

		state_ = STATE_OPEN;
		emit Connected();
	}

			    
	void Connection::OnNewChannels(const Tp::ChannelDetailsList& channels)
	{
		LogInfo("New channel received.");
		foreach (const Tp::ChannelDetails &details, channels) 
		{
			QString channelType = details.properties.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".ChannelType")).toString();
			bool requested = details.properties.value(QLatin1String(TELEPATHY_INTERFACE_CHANNEL ".Requested")).toBool();
			// qDebug() << " channelType:" << channelType;
			//  qDebug() << " requested  :" << requested;

			if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_TEXT && !requested)
			{
				LogInfo("Text chat request received.");

				Tp::TextChannelPtr channel = Tp::TextChannel::create(tp_connection_, details.channel.path(), details.properties);
				LogInfo("Text channel object created.");
				
				//mCallHandler->addIncomingCall(channel);
				ChatSessionRequest* request = new ChatSessionRequest(channel); 
				received_text_chat_requests_.push_back(request);
				
				QObject::connect(request, SIGNAL( Ready(ChatSessionRequest*) ), SLOT( OnIncomingChatSessionRequestReady(ChatSessionRequest* ) ) );
			}

			if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_CONTACT_LIST && !requested)
			{
				LogInfo("Contact list channel");
			}

			if (channelType == TELEPATHY_INTERFACE_CHANNEL_TYPE_STREAMED_MEDIA && !requested)
			{
				LogInfo("Streamed media channel");
			}
		}
	}

	void Connection::OnIncomingChatSessionRequestReady(ChatSessionRequest* request)
	{
		LogInfo("emit ReceivedChatSessionRequest(request)");
		emit ReceivedChatSessionRequest(request);
	}

	void Connection::OnPresencePublicationRequested(const Tp::Contacts &contacts)
	{
		foreach (const Tp::ContactPtr &contact, contacts)
		{
			QString message = "OnPresencePublicationRequested from ";
			message.append( contact->id() );
			LogInfo(message.toStdString());

			ContactVector current_contacts = this->user_->GetContacts();
			for (ContactVector::iterator i = current_contacts.begin(); i != current_contacts.end(); ++i)
			{
				if ( (*i)->GetAddress().compare( contact->id().toStdString() ) == 0)
				{
					// The contact is already in contact list
				}
			}

			FriendRequest* request = new FriendRequest(Tp::ContactPtr(contact));
			received_friend_requests_.push_back(request);
			emit ReceivedFriendRequest(request);
		}
	}
	
	void Connection::OnContactRetrievedForFriendRequest(Tp::PendingOperation *op)
	{
		if (op->isError())
		{
			QString message = "Failed to receive contact: ";
			message.append(op->errorMessage());
			LogError(message.toStdString());
			return;
		}
		LogInfo("Contact received.");
		Tp::PendingContacts *pending_contacts = qobject_cast<Tp::PendingContacts *>(op);
		QList<Tp::ContactPtr> contacts = pending_contacts->contacts();
		
		assert( contacts.size() == 1); // We have request only one contact 
		Tp::ContactPtr contact = contacts.first();
		QString message = ""; // todo

		// Do the presence subscription
		// When we get the answer ?
		Tp::PendingOperation* p = contact->requestPresenceSubscription(message);
		connect(p, SIGNAL(finished(Tp::PendingOperation*)), SLOT(OnPresenceSubscriptionResult(Tp::PendingOperation*)));
	}

	void Connection::OnPresenceSubscriptionResult(Tp::PendingOperation* op)
	{
		if (op->isError())
		{
			QString message = "Canoot subscribe presence: ";
			message.append(op->errorMessage());
			LogInfo(message.toStdString());

			// Presence subscription didn't success, we emit a signal about it
			QString to = ""; // todo: get the right value
			emit FriendRequestRejected(to);
		}
		LogInfo("Presence subscribed successfully.");
		// Now it's our turn to publish our status
		
		// From where to get contact object ???
		// todo: move this logic to Contact class..
	}

	void Connection::OnConnectionInvalidated(Tp::DBusProxy *proxy, const QString &errorName, const QString &errorMessage)
	{
		LogError("Connection::OnConnectionInvalidated");
		state_ = STATE_ERROR;
		QString message = "Connection become invalitated.";
		emit Error(message);
	}
	
	ChatSessionRequestVector Connection::GetChatSessionRequests()
	{
		ChatSessionRequestVector requests;
		requests.assign(received_text_chat_requests_.begin(), received_text_chat_requests_.end());
		return requests;
	}

	FriendRequestVector Connection::GetFriendRequests()
	{
		return FriendRequestVector(received_friend_requests_);
	}

	ChatSessionVector Connection::GetChatSessions()
	{
		return ChatSessionVector(text_chat_sessions_);
	}

	PresenceStatusOptions Connection::GetAvailablePresenceStatusOptions()
	{
		PresenceStatusOptions options;
		Tp::SimpleStatusSpecMap map = tp_connection_->allowedPresenceStatuses();
		for (Tp::SimpleStatusSpecMap::iterator i = map.begin(); i != map.end(); ++i)
		{
			QString o = i.key();
			if ( o.compare("offline") == 0 || o.compare("unknown") == 0 || o.compare("error") == 0 )
				continue; // HACK: Gabble crash if presence status is set to 'offline', 'unknown' or 'error'
			options.push_back( o.toStdString() );
			
			QString message = "Presence status option: ";
			message.append(o);
			LogInfo(message.toStdString());
		}
		return options;
	}

	void Connection::SendFriendRequest(const Address &to, const std::string &message)
	{
		Tp::PendingContacts *pending_contacts = tp_connection_->contactManager()->contactsForIdentifiers(QStringList()<<QString(to.c_str()));
		connect(pending_contacts, SIGNAL( finished(Tp::PendingOperation *) ), SLOT( OnContactRetrievedForFriendRequest(Tp::PendingOperation *) ));
	}
} // end of namespace: TpQt4Communication
