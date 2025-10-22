"""
Manual summarization fallback for when Gemini API is unavailable.
This module provides a basic text summarization algorithm when API-based summarization fails.
"""

def simple_summarize(text, max_length=80):
    """
    Create a simple summary of the given text.
    
    This function attempts to create a meaningful summary by:
    1. Using the first sentence if it's short enough
    2. Taking important sentences based on keyword frequency
    3. Falling back to truncation if necessary
    
    Args:
        text (str): The text to summarize
        max_length (int): Maximum length of the summary
        
    Returns:
        str: A summary of the text no longer than max_length characters
    """
    print("Using simple fallback summarization algorithm")
    # If text is already short enough, return it as is
    if len(text) <= max_length:
        return text
        
    # Try to get the first sentence
    sentences = text.split('.')
    first_sentence = sentences[0].strip()
    
    # If first sentence is short enough, use it
    if len(first_sentence) <= max_length:
        return first_sentence
    
    # Otherwise, try to find important keywords
    words = text.lower().split()
    # Remove common words
    stop_words = {'the', 'a', 'an', 'and', 'or', 'but', 'is', 'are', 'was', 'were', 
                 'in', 'on', 'at', 'to', 'for', 'with', 'by', 'about', 'of', 'from'}
    keywords = [word for word in words if word not in stop_words]
    
    # Count word frequency
    word_freq = {}
    for word in keywords:
        if word in word_freq:
            word_freq[word] += 1
        else:
            word_freq[word] = 1
    
    # Get most frequent words
    top_words = sorted(word_freq.items(), key=lambda x: x[1], reverse=True)[:5]
    top_words = [word for word, _ in top_words]
    
    # Try to create a summary with these key words
    summary = f"Email about {', '.join(top_words[:3])}"
    
    # If still too long, just truncate
    if len(summary) > max_length:
        return text[:max_length-3] + "..."
        
    return summary