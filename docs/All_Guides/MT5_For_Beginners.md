# MT5 for Beginners: Your First Steps in Trading

Welcome! This guide is for complete beginners who have never worked with MetaTrader 5 or trading. We'll get you from zero to a working demo account you can automate with CppMT5.

---

## What is MetaTrader 5 (MT5)?

**MetaTrader 5** is a trading platform used by millions of traders worldwide. It lets you:

- Trade forex, stocks, commodities, and cryptocurrencies
- Analyze markets with charts and indicators
- Execute automated trading strategies
- Monitor your positions in real-time

Think of MT5 as the "cockpit" for the markets. **MetaRPC** runs real MT5 terminals in the cloud and **CppMT5** lets your C++ program drive one over gRPC. So the chain is:

```
your C++ program  ──▶  CppMT5  ──gRPC──▶  MetaRPC gateway  ──▶  an MT5 terminal  ──▶  your broker
```

---

## Demo vs Live account: start safely

A **demo account** is a risk-free practice account with virtual money — **start here**:

- ✅ **Zero risk** — virtual funds (typically $10,000 – $100,000)
- ✅ **Real market conditions** — live prices and behavior
- ✅ **Learn the platform** — before risking real money
- ✅ **Test your code** — perfect for developing CppMT5 apps

A **live account** uses real money. Only go live once you understand MT5, have tested thoroughly on demo, and understand the risks.

---

## The vocabulary you need

- **Symbol** — a tradable instrument, e.g. `EURUSD`, `XAUUSD`.
- **Bid / Ask** — the price to **sell** at / **buy** at. Their gap is the **spread**.
- **Lot / volume** — trade size. `0.01` lots is the usual minimum ("a micro lot").
- **Position** — an open trade. It has a direction (BUY/SELL), a volume, and a floating profit.
- **Pending order** — an order waiting at a price to become a position (limit/stop).
- **SL / TP** — *stop loss* and *take profit*: prices at which the position auto-closes to cap a loss or lock a gain.
- **Balance vs Equity** — balance is settled money; equity is balance plus the profit/loss of what is currently open.
- **Margin / free margin** — money the broker reserves to hold your positions, and what's left to open more.

See the full [Glossary](GLOSSARY.md).

### Order vs position vs deal

MT5 keeps these three separate — it trips up almost everyone at first:

- An **order** is the *instruction* ("buy 0.1 lots of EURUSD").
- Once it fills, you hold a **position** — the live exposure that shows a floating profit.
- Every execution is recorded in history as a **deal** — that's where *realized* profit lives.

A single market buy creates an order → a position → a deal. A **pending order** sits as just an order until its price is reached.

---

## Quick start: creating a demo account

The fastest way is to create a demo account directly in the MT5 terminal.

### Step 1️⃣ — Download and install the MT5 terminal

1. Visit [https://www.metatrader5.com/en/download](https://www.metatrader5.com/en/download)
2. Download the version for your OS (Windows / macOS / Linux)
3. Install the application

### Step 2️⃣ — Launch MT5 and find the Navigator panel

On first launch a popup usually offers to open a new account. If not, find the **Navigator** panel (left side, below **Market Watch**).

![Navigator Panel Location](../Guide_Images/1.%20NAV.png)

Inside Navigator you'll see **Accounts**, Indicators, Expert Advisors, and more.

### Step 3️⃣ — Open a new demo account

1. In **Navigator**, find **Accounts**
2. **Right-click** "Accounts"
3. Select **"Open an account"**

### Step 4️⃣ — Select the MetaQuotes demo server

In the broker search window, look for **"MetaQuotes-Demo"** (the standard demo server). Use the search field if needed, select it, and click **Next**.

![MetaQuotes Server Selection](../Guide_Images/2.%20META.png)

### Step 5️⃣ — Account type

Choose **"Open a new demo account"** and click **Next**.

![Open New Demo Account](../Guide_Images/3.%20OPEN.png)

### Step 6️⃣ — Fill in your details

Name, email, account type, virtual deposit (e.g. $10,000), leverage (1:100 is fine to start), currency. Agree to the terms and click **Next**.

![Personal Information Form](../Guide_Images/4.%20creator.png)

### Step 7️⃣ — Save your credentials

MT5 creates the account and shows:

- **Login** — your account number (e.g. 5057947437)
- **Password** — your master (trading) password
- **Investor Password** — read-only password
- **Server** — e.g. "MetaQuotes-Demo"

**⚠️ Save these now** — you need **Login**, **Password**, and **Server** to connect from CppMT5. Click **Finish**.

![Account Credentials](../Guide_Images/5.%20OKK.png)

---

### Step 8️⃣ — Connect from CppMT5

You also need a MetaRPC API key (free) — sign up at [https://mrpc.pro/signup](https://mrpc.pro/signup) and generate one at [https://mrpc.pro/my](https://mrpc.pro/my). Provide it via the `MRPC_API_KEY` environment variable (or the constructor / `config.json`):

```bash
export MRPC_API_KEY=your_mrpc_token    # from https://mrpc.pro/my
```

Then pass your MT5 credentials straight to the SDK:

```cpp
#include <iostream>
#include <metarpc/mt5_sugar.hpp>
using namespace metarpc;

int main() {
    MT5Sugar sugar("mt5.mrpc.pro", 443);                 // default endpoint
    sugar.connect(5057947437, "YourPasswordHere", "MetaQuotes-Demo");   // exact server name

    std::cout << "balance " << sugar.getBalance() << "\n";
    std::cout << "EURUSD  " << sugar.getBid("EURUSD") << " / " << sugar.getAsk("EURUSD") << "\n";

    sugar.disconnect();
}
```

- **Login / password** — exactly what MT5 showed you.
- **Server name** — the exact string (e.g. `"MetaQuotes-Demo"`).
- **endpoint / API key** — `"mt5.mrpc.pro", 443` is the default; the key comes from `MRPC_API_KEY`.

### Step 9️⃣ — Verify the MT5 connection

In the MT5 terminal, the **bottom-right corner** should show a green connection indicator and your balance (e.g. $10,000).

### Step 🔟 — Build and run

```bash
cmake -G Ninja -B build && cmake --build build
./build/my_bot
```

Trades your program makes appear in the MT5 terminal in real time. It's virtual money — experiment freely. See [Your First Project](Your_First_Project.md) for a fuller walkthrough.

---

## Your first five calls

Everything below uses `MT5Sugar`, the friendliest layer:

```cpp
MT5Sugar sugar("mt5.mrpc.pro", 443);
sugar.connect(5057947437, "YourPassword", "MetaQuotes-Demo");   // 1. connect

double bal = sugar.getBalance();              // 2. how much money?
double bid = sugar.getBid("EURUSD");          // 3. what's the price?
uint64_t t = sugar.buyMarket("EURUSD", 0.01); // 4. buy a micro lot
sugar.closePosition((int64_t)t);              // 5. close it
```

!!! warning "Did the order actually fill?"
    A call returning without an error is **not** the same as a filled trade. `buyMarket` returns
    the new **ticket**, or `0` if the broker rejected it — always check it. The friendly
    `orderSend` returns an `OrderResult` whose `retcode == 10009` means *done*. See
    [Return Codes](RETURN_CODES_REFERENCE.md).

---

## 🧩 MT5 password types

**Master password** — full access (open/close trades, change settings). **This is the one CppMT5 uses to trade.**

**Investor password** — view-only: see trades/balance/history but cannot trade. Good for sharing performance or monitoring. Set it via **Tools → Options → Server → Change** (Investor Password).

---

## Choosing a broker (optional)

The MetaQuotes demo server is fine for practice. For live trading later you may pick a specific broker for better conditions or regional support.

**Not financial advice — do your own research.** Popular MT5 brokers include IC Markets, Pepperstone, OANDA, XM, RoboForex, FxPro.

**What to look for:** regulation (FCA/ASIC/CySEC/…), genuine MT5 support (not just MT4), a free demo, tight spreads, responsive support.

---

## Important reminders

- 🛡️ **Security** — never share your **master password**; use the investor password for monitoring tools. Keep credentials out of your repo (don't commit `config.json` with real values).
- 📊 **Risk management** — even on demo, build good habits: always use a stop-loss, risk ≤ 1–2% per trade, size with [`MT5Sugar::calculatePositionSize`](../MT5Sugar/10.%20Risk_Management.md).
- 🧪 **Testing** — test on **demo first**; verify strategies over weeks/months, not days. Demo results don't guarantee live results.
- 📚 **Education** — trading has a steep learning curve; focus on learning, not earning.

---

## What's next?

1. [Getting Started](GETTING_STARTED.md) — install, build, get an API key, run a demo.
2. [Your First Project](Your_First_Project.md) — a complete little program.
3. Understand the layers: [MT5Account](../MT5Account/MT5Account.Master.Overview.md) (low-level) · [MT5Service](../MT5Service/MT5Service.Overview.md) (friendly) · [MT5Sugar](../MT5Sugar/MT5Sugar.API_Overview.md) (one-liners, most beginner-friendly).
4. [Strategies](../Strategies/Strategies.Master.Overview.md) — ready orchestrators and a preset.
5. [Glossary](GLOSSARY.md) · [Project Map](PROJECT_MAP.md) — the vocabulary and the lay of the land.

Good luck on your trading journey! 🚀
