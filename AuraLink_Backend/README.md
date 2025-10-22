# AuraLink Gmail Summary

A console application that authenticates with Google Gmail, continuously checks for unread emails in the last 30 minutes, reads them without marking as read, and creates summaries using Google's Gemini AI.

## Features

- Google Gmail authentication with OAuth 2.0
- Continuously checks for unread emails from the last 30 minutes in real-time
- Polls for unread emails every 60 seconds
- Only processes emails received within the last 30 minutes (ignores older emails)
- Reads email content without marking messages as read
- Summarizes email content using Google's Gemini AI free tier models
- Automatic model discovery and selection from available Gemini models
- Smart fallback to custom summarization algorithm if Gemini API fails
- Console-based user interface
- Graceful exit with Ctrl+C

## Setup

### Prerequisites

- Python 3.7 or higher
- A Google account
- Google Cloud project with Gmail API enabled
- Google Cloud OAuth 2.0 client credentials

### Installation

1. Clone this repository:
```
git clone https://github.com/yourusername/auralink_backend.git
cd auralink_backend
```

2. Install required dependencies:
```
pip install -r requirements.txt
```

3. Set up Google Cloud project:
   - Visit the [Google Cloud Console](https://console.cloud.google.com/)
   - Create a new project or select an existing one
   - Enable the Gmail API
   - Create OAuth 2.0 credentials (Desktop client)
   - Download the credentials JSON file
   - Rename and save as `credentials.json` in the project root directory

## Usage

Run the application:
```
python gmail_summary.py
```

On first run, the application will:
1. Open a browser window for Google authentication
2. Request permission to access your Gmail account
3. Save authentication token locally for future use

The application will then:
1. Start continuous monitoring for unread emails
2. Check for unread emails every 60 seconds
3. For each check:
   - Find the most recent unread email
   - Verify it was received within the last 30 minutes (ignores older emails)
   - If a valid unread email is found:
     - Display sender, subject, and receive time
     - Generate an 80-character summary using Gemini AI
     - Remember processed emails to avoid duplicate summaries
   - If no valid unread emails are found within the 30-minute window, show "None found"
4. Continue running until you press Ctrl+C to exit

## Technical Details

### Gemini AI Integration
- Uses the latest Gemini 2.5 free tier models (Flash-Lite, Flash, Pro)
- Auto-discovers available models using the Gemini API
- Selects the best available model based on predefined preferences
- Includes a fallback custom summarization algorithm if Gemini API is unavailable

### Security Notes
- The application stores authentication tokens locally in `token.pickle`
- The application uses read-only access to Gmail, ensuring emails are not marked as read
- The Gemini API key is included directly in the code for demonstration purposes, but in a production environment, it should be stored securely (e.g., environment variables)

## License

This project is licensed under the MIT License - see the LICENSE file for details.