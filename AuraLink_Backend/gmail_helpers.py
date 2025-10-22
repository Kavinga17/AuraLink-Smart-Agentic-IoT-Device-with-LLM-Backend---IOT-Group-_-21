def get_any_unread_emails(service):
    """Get any unread emails sorted by date (newest first)"""
    query = "is:unread"
    
    # Call the Gmail API - sort by date so newest messages come first
    results = service.users().messages().list(userId='me', q=query, maxResults=10).execute()
    messages = results.get('messages', [])
    
    return messages