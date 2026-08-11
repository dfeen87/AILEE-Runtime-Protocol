# AILEE Web Dashboard

## Overview

This directory contains the web-based dashboard for AILEE Protocol Core. The dashboard provides a real-time view of the Bitcoin Layer-2 node's status, performance metrics, and Layer-2 state.

## Files

- `index.html` - Main dashboard interface with real-time monitoring

## Features

- **Real-time Monitoring**: Auto-refreshes every 10 seconds
- **Responsive Design**: Works on desktop, tablet, and mobile devices
- **Clean Interface**: Modern UI with gradient backgrounds and card-based layout
- **API Integration**: Connects to the AILEE REST API for live data

## Usage

The web dashboard is automatically served by the AILEE web server when you run:

```bash
./ailee_web_demo
```

Then open your browser to: `http://localhost:8080/`

## Customization

You can customize the dashboard by editing `index.html`. The file is self-contained with inline CSS and JavaScript for easy modification.

### Changing the Refresh Interval

To change how often the dashboard updates, modify this line in `index.html`:

```javascript
setInterval(loadAllData, 10000);  // Change 10000 to desired milliseconds
```

### Changing the API Base URL

If your AILEE node is running on a different host or port, update:

```javascript
const API_BASE = window.location.origin;  // Change to custom URL if needed
```

## Browser Compatibility

The dashboard works with modern browsers that support:
- ES6 JavaScript
- Fetch API
- CSS Grid and Flexbox

Tested on:
- Chrome 90+
- Firefox 88+
- Safari 14+
- Edge 90+

## Development

To modify the dashboard:

1. Edit `web/index.html`
2. Reload the page in your browser
3. No build step required - it's pure HTML/CSS/JavaScript

## API Endpoints Used

The dashboard consumes these API endpoints:

- `GET /api/status` - Node status and statistics
- `GET /api/metrics` - Performance metrics  
- `GET /api/l2/state` - Layer-2 state information

See [`../docs/WEB_INTEGRATION.md`](../docs/WEB_INTEGRATION.md) for complete API documentation.

## License

MIT License - Same as AILEE Protocol Core
